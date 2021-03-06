/****************************************************************************
*
*    Copyright (C) 2005 - 2014 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/


#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_allocator.h"
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mman.h>
#include <asm/atomic.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "gc_hal_kernel_allocator_array.h"
#include "gc_hal_kernel_platform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

static void
_NonContiguousFree(
    IN struct page ** Pages,
    IN gctUINT32 NumPages
    )
{
    gctINT i;

    gcmkHEADER_ARG("Pages=0x%X, NumPages=%d", Pages, NumPages);

    gcmkASSERT(Pages != gcvNULL);

    for (i = 0; i < NumPages; i++)
    {
        __free_page(Pages[i]);
    }

    if (is_vmalloc_addr(Pages))
    {
        vfree(Pages);
    }
    else
    {
        kfree(Pages);
    }

    gcmkFOOTER_NO();
}

static struct page **
_NonContiguousAlloc(
    IN gctUINT32 NumPages
    )
{
    struct page ** pages;
    struct page *p;
    gctINT i, size;

    gcmkHEADER_ARG("NumPages=%lu", NumPages);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
    if (NumPages > totalram_pages)
#else
    if (NumPages > num_physpages)
#endif
    {
        gcmkFOOTER_NO();
        return gcvNULL;
    }

    size = NumPages * sizeof(struct page *);

    pages = kmalloc(size, GFP_KERNEL | gcdNOWARN);

    if (!pages)
    {
        pages = vmalloc(size);

        if (!pages)
        {
            gcmkFOOTER_NO();
            return gcvNULL;
        }
    }

    for (i = 0; i < NumPages; i++)
    {
        p = alloc_page(GFP_KERNEL | __GFP_HIGHMEM | gcdNOWARN);

        if (!p)
        {
            _NonContiguousFree(pages, i);
            gcmkFOOTER_NO();
            return gcvNULL;
        }

        pages[i] = p;
    }

    gcmkFOOTER_ARG("pages=0x%X", pages);
    return pages;
}

gctSTRING
_CreateKernelVirtualMapping(
    IN PLINUX_MDL Mdl
    )
{
    gctSTRING addr = 0;
    gctINT numPages = Mdl->numPages;

#if gcdNONPAGED_MEMORY_CACHEABLE
    if (Mdl->contiguous)
    {
        addr = page_address(Mdl->u.contiguousPages);
    }
    else
    {
        addr = vmap(Mdl->u.nonContiguousPages,
                    numPages,
                    0,
                    PAGE_KERNEL);

        /* Trigger a page fault. */
        memset(addr, 0, numPages * PAGE_SIZE);
    }
#else
    struct page ** pages;
    gctBOOL free = gcvFALSE;
    gctINT i;

    if (Mdl->contiguous)
    {
        pages = kmalloc(sizeof(struct page *) * numPages, GFP_KERNEL | gcdNOWARN);

        if (!pages)
        {
            return gcvNULL;
        }

        for (i = 0; i < numPages; i++)
        {
            pages[i] = nth_page(Mdl->u.contiguousPages, i);
        }

        free = gcvTRUE;
    }
    else
    {
        pages = Mdl->u.nonContiguousPages;
    }

    /* ioremap() can't work on system memory since 2.6.38. */
    addr = vmap(pages, numPages, 0, gcmkNONPAGED_MEMROY_PROT(PAGE_KERNEL));

    if (free)
    {
        kfree(pages);
    }

#endif

    return addr;
}

void
_DestoryKernelVirtualMapping(
    IN gctSTRING Addr
    )
{
#if !gcdNONPAGED_MEMORY_CACHEABLE
    vunmap(Addr);
#endif
}

void
_UnmapUserLogical(
    IN gctPOINTER Logical,
    IN gctUINT32  Size
)
{
    if (unlikely(current->mm == gcvNULL))
    {
        /* Do nothing if process is exiting. */
        return;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    if (vm_munmap((unsigned long)Logical, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): vm_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
#else
    down_write(&current->mm->mmap_sem);
    if (do_munmap(current->mm, (unsigned long)Logical, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): do_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
    up_write(&current->mm->mmap_sem);
#endif
}

/***************************************************************************\
************************ Default Allocator **********************************
\***************************************************************************/
#define C_MAX_PAGENUM  (50*1024)

static gceSTATUS
_DefaultAlloc(
    IN gckALLOCATOR Allocator,
    INOUT PLINUX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;
    gctUINT32 order;
    gctSIZE_T bytes;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
    gctPOINTER addr = gcvNULL;
#endif
    gctUINT32 numPages;
    gctUINT i = 0;
    gctBOOL contiguous = Flags & gcvALLOC_FLAG_CONTIGUOUS;

    struct sysinfo temsysinfo;

    gcmkHEADER_ARG("Mdl=%p NumPages=%d", Mdl, NumPages);

    numPages = NumPages;
    bytes = NumPages * PAGE_SIZE;
    order = get_order(bytes);
    si_meminfo(&temsysinfo);
    if ((Flags & gcvALLOC_FLAG_MEMLIMIT))
    {
        if ( (temsysinfo.freeram < NumPages) || ((temsysinfo.freeram-NumPages) < C_MAX_PAGENUM) )
        {
            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
    }
     
    if (contiguous)
    {
        if (order >= MAX_ORDER)
        {
            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        addr =
            alloc_pages_exact(bytes, GFP_KERNEL | gcdNOWARN | __GFP_NORETRY);

        Mdl->u.contiguousPages = addr
                               ? virt_to_page(addr)
                               : gcvNULL;

        Mdl->exact = gcvTRUE;
#else
        Mdl->u.contiguousPages =
            alloc_pages(GFP_KERNEL | gcdNOWARN | __GFP_NORETRY, order);
#endif

        if (Mdl->u.contiguousPages == gcvNULL)
        {
            Mdl->u.contiguousPages =
                alloc_pages(GFP_KERNEL | __GFP_HIGHMEM | gcdNOWARN, order);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
            Mdl->exact = gcvFALSE;
#endif
        }
    }
    else
    {
       Mdl->u.nonContiguousPages = _NonContiguousAlloc(numPages);
    }

    if (Mdl->u.contiguousPages == gcvNULL && Mdl->u.nonContiguousPages == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    for (i = 0; i < numPages; i++)
    {
        struct page *page;

        if (contiguous)
        {
            page = nth_page(Mdl->u.contiguousPages, i);
        }
        else
        {
            page = _NonContiguousToPage(Mdl->u.nonContiguousPages, i);
        }

        SetPageReserved(page);

        if (!PageHighMem(page) && page_to_phys(page))
        {
            gcmkVERIFY_OK(
                gckOS_CacheFlush(Allocator->os, _GetProcessID(), gcvNULL,
                                 page_to_phys(page),
                                 page_address(page),
                                 PAGE_SIZE));
        }
        else
        {
            flush_dcache_page(page);

#if !gcdCACHE_FUNCTION_UNIMPLEMENTED && defined(CONFIG_OUTER_CACHE) && gcdENABLE_OUTER_CACHE_PATCH
            if (page_to_phys(page))
            {
                _HandleOuterCache(
                    Allocator->os,
                    page_to_phys(page),
                    gcvNULL,
                    PAGE_SIZE,
                    gcvCACHE_FLUSH
                    );
            }
#endif
        }
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

static void
_DefaultFree(
    IN gckALLOCATOR Allocator,
    IN OUT PLINUX_MDL Mdl
    )
{
    gctINT i;

    for (i = 0; i < Mdl->numPages; i++)
    {
        if (Mdl->contiguous)
        {
            ClearPageReserved(nth_page(Mdl->u.contiguousPages, i));
        }
        else
        {
            ClearPageReserved(_NonContiguousToPage(Mdl->u.nonContiguousPages, i));
        }
    }

    if (Mdl->contiguous)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        if (Mdl->exact == gcvTRUE)
        {
            free_pages_exact(page_address(Mdl->u.contiguousPages), Mdl->numPages * PAGE_SIZE);
        }
        else
#endif
        {
            __free_pages(Mdl->u.contiguousPages, get_order(Mdl->numPages * PAGE_SIZE));
        }
    }
    else
    {
        _NonContiguousFree(Mdl->u.nonContiguousPages, Mdl->numPages);
    }
}

gctINT
_DefaultMapUser(
    gckALLOCATOR Allocator,
    PLINUX_MDL Mdl,
    PLINUX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{

    gctSTRING       addr;
    unsigned long   start;
    unsigned long   pfn;
    gctINT i;
    gckOS           os = Allocator->os;
    gcsPLATFORM *   platform = os->device->platform;

    PLINUX_MDL      mdl = Mdl;
    PLINUX_MDL_MAP  mdlMap = MdlMap;

    gcmkHEADER_ARG("Allocator=%p Mdl=%p MdlMap=%p gctBOOL=%d", Allocator, Mdl, MdlMap, Cacheable);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    mdlMap->vmaAddr = (gctSTRING)vm_mmap(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);
#else
    down_write(&current->mm->mmap_sem);

    mdlMap->vmaAddr = (gctSTRING)do_mmap_pgoff(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);

    up_write(&current->mm->mmap_sem);
#endif

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_OS,
        "%s(%d): vmaAddr->0x%X for phys_addr->0x%X",
        __FUNCTION__, __LINE__,
        (gctUINT32)(gctUINTPTR_T)mdlMap->vmaAddr,
        (gctUINT32)(gctUINTPTR_T)mdl
        );

    if (IS_ERR(mdlMap->vmaAddr))
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): do_mmap_pgoff error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    down_write(&current->mm->mmap_sem);

    mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

    if (mdlMap->vma == gcvNULL)
    {
        up_write(&current->mm->mmap_sem);

        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): find_vma error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }




    if (platform && platform->ops->adjustProt)
    {
        platform->ops->adjustProt(mdlMap->vma);
    }

    addr = mdl->addr;

    /* Now map all the vmalloc pages to this user address. */
    if (mdl->contiguous)
    {
        /* map kernel memory to user space.. */
        mdlMap->vma->vm_flags |= gcdVM_FLAGS;

        if (Cacheable == gcvFALSE)
        {
            /* Make this mapping non-cached. */
            mdlMap->vma->vm_page_prot = gcmkPAGED_MEMROY_PROT(mdlMap->vma->vm_page_prot);
        }

        if (remap_pfn_range(mdlMap->vma,
                            mdlMap->vma->vm_start,
                            page_to_pfn(mdl->u.contiguousPages),
                            mdlMap->vma->vm_end - mdlMap->vma->vm_start,
                            mdlMap->vma->vm_page_prot) < 0)
        {
            up_write(&current->mm->mmap_sem);

            gcmkTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_OS,
                "%s(%d): unable to mmap ret",
                __FUNCTION__, __LINE__
                );

            mdlMap->vmaAddr = gcvNULL;

            gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
            return gcvSTATUS_OUT_OF_MEMORY;
        }
    }
    else
    {
        mdlMap->vma->vm_flags |= gcdVM_FLAGS;

        if (Cacheable == gcvFALSE)
        {
            /* Make this mapping non-cached. */
            mdlMap->vma->vm_page_prot = gcmkPAGED_MEMROY_PROT(mdlMap->vma->vm_page_prot);
        }

        start = mdlMap->vma->vm_start;

        for (i = 0; i < mdl->numPages; i++)
        {
            pfn = _NonContiguousToPfn(mdl->u.nonContiguousPages, i);

            if (remap_pfn_range(mdlMap->vma,
                                start,
                                pfn,
                                PAGE_SIZE,
                                mdlMap->vma->vm_page_prot) < 0)
            {
                up_write(&current->mm->mmap_sem);

                mdlMap->vmaAddr = gcvNULL;

                gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
                return gcvSTATUS_OUT_OF_MEMORY;
            }

            start += PAGE_SIZE;
            addr += PAGE_SIZE;
        }
    }

    up_write(&current->mm->mmap_sem);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

void
_DefaultUnmapUser(
    IN gckALLOCATOR Allocator,
    IN gctPOINTER Logical,
    IN gctUINT32 Size
    )
{
    _UnmapUserLogical(Logical, Size);
}

gceSTATUS
_DefaultMapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    OUT gctPOINTER *Logical
    )
{
    *Logical = _CreateKernelVirtualMapping(Mdl);
    return gcvSTATUS_OK;
}

gceSTATUS
_DefaultUnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical
    )
{
    _DestoryKernelVirtualMapping(Logical);
    return gcvSTATUS_OK;
}

gceSTATUS
_DefaultLogicalToPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    OUT gctUINT32_PTR Physical
    )
{
    return _ConvertLogical2Physical(
                Allocator->os, Logical, ProcessID, Mdl, Physical);
}

gceSTATUS
_DefaultCache(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
_DefaultPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctUINT32_PTR Physical
    )
{
    gcmkASSERT(Mdl->pagedMem && !Mdl->contiguous);
    *Physical = _NonContiguousToPhys(Mdl->u.nonContiguousPages, Offset);

    return gcvSTATUS_OK;
}

/* Default allocator operations. */
gcsALLOCATOR_OPERATIONS DefaultAllocatorOperations = {
    .Alloc              = _DefaultAlloc,
    .Free               = _DefaultFree,
    .MapUser            = _DefaultMapUser,
    .UnmapUser          = _DefaultUnmapUser,
    .MapKernel          = _DefaultMapKernel,
    .UnmapKernel        = _DefaultUnmapKernel,
    .LogicalToPhysical  = _DefaultLogicalToPhysical,
    .Cache              = _DefaultCache,
    .Physical           = _DefaultPhysical,
};

/* Default allocator entry. */
gceSTATUS
_DefaultAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator;

    gcmkONERROR(
        gckALLOCATOR_Construct(Os, &DefaultAllocatorOperations, &allocator));

    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    return status;
}
#if LINUX_CMA_FSL

static gceSTATUS
_CMAFSLAlloc(
    IN gckALLOCATOR Allocator,
    INOUT PLINUX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;
    gctPOINTER addr = gcvNULL;

    gcmkHEADER_ARG("Mdl=%p NumPages=%d", Mdl, NumPages);

    addr = dma_alloc_writecombine(gcvNULL,
            NumPages * PAGE_SIZE,
            &Mdl->dmaHandle,
            GFP_KERNEL | gcdNOWARN);

    if (addr == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    Mdl->addr = addr;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

static void
_CMAFSLFree(
    IN gckALLOCATOR Allocator,
    IN OUT PLINUX_MDL Mdl
    )
{
    dma_free_writecombine(gcvNULL,
            Mdl->numPages * PAGE_SIZE,
            Mdl->addr,
            Mdl->dmaHandle);
}

gctINT
_CMAFSLMapUser(
    gckALLOCATOR Allocator,
    PLINUX_MDL Mdl,
    PLINUX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{

    gctSTRING       addr;
    PLINUX_MDL      mdl = Mdl;
    PLINUX_MDL_MAP  mdlMap = MdlMap;

    gcmkHEADER_ARG("Allocator=%p Mdl=%p MdlMap=%p gctBOOL=%d", Allocator, Mdl, MdlMap, Cacheable);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    mdlMap->vmaAddr = (gctSTRING)vm_mmap(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);
#else
    down_write(&current->mm->mmap_sem);

    mdlMap->vmaAddr = (gctSTRING)do_mmap_pgoff(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);

    up_write(&current->mm->mmap_sem);
#endif

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_OS,
        "%s(%d): vmaAddr->0x%X for phys_addr->0x%X",
        __FUNCTION__, __LINE__,
        (gctUINT32)(gctUINTPTR_T)mdlMap->vmaAddr,
        (gctUINT32)(gctUINTPTR_T)mdl
        );

    if (IS_ERR(mdlMap->vmaAddr))
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): do_mmap_pgoff error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    down_write(&current->mm->mmap_sem);

    mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

    if (mdlMap->vma == gcvNULL)
    {
        up_write(&current->mm->mmap_sem);

        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): find_vma error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    addr = mdl->addr;

    /* Now map all the vmalloc pages to this user address. */
    if (mdl->contiguous && (!mdl->cacheable))
    {
        /* map kernel memory to user space.. */
        if (dma_mmap_writecombine(gcvNULL,
                mdlMap->vma,
                mdl->addr,
                mdl->dmaHandle,
                mdl->numPages * PAGE_SIZE) < 0)
        {
            up_write(&current->mm->mmap_sem);

            gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): dma_mmap_attrs error",
                __FUNCTION__, __LINE__
                );

             mdlMap->vmaAddr = gcvNULL;

            gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
            return gcvSTATUS_OUT_OF_MEMORY;
        }
    }
    else
    {
        gckOS_Print("incorrect mdl:conti%d:cachable:%d\n",mdl->contiguous, mdl->cacheable);        
    }

    up_write(&current->mm->mmap_sem);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}



/* Default allocator operations. */
gcsALLOCATOR_OPERATIONS CMAFSLAllocatorOperations = {
    .Alloc              = _CMAFSLAlloc,
    .Free               = _CMAFSLFree,
    .MapUser            = _CMAFSLMapUser,
    .UnmapUser          = _DefaultUnmapUser,
    .MapKernel          = _DefaultMapKernel,
    .UnmapKernel        = _DefaultUnmapKernel,
    .LogicalToPhysical  = _DefaultLogicalToPhysical,
    .Cache              = _DefaultCache,
    .Physical           = _DefaultPhysical,
};

/* Default allocator entry. */
gceSTATUS
_CMAFSLAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator;

    gcmkONERROR(
        gckALLOCATOR_Construct(Os, &CMAFSLAllocatorOperations, &allocator));

    allocator->privateData = (void *)1;
        
    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    return status;
}

#endif
/***************************************************************************\
************************ Allocator helper ***********************************
\***************************************************************************/

gceSTATUS
gckALLOCATOR_Construct(
    IN gckOS Os,
    IN gcsALLOCATOR_OPERATIONS * Operations,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator;

    gcmkHEADER_ARG("Os=%p, Operations=%p, Allocator=%p",
                   Os, Operations, Allocator);

    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Allocator != gcvNULL);
    gcmkVERIFY_ARGUMENT
        (  Operations
        && Operations->Alloc
        && Operations->Free
        && Operations->MapUser
        && Operations->UnmapUser
        && Operations->MapKernel
        && Operations->UnmapKernel
        && Operations->LogicalToPhysical
        && Operations->Cache
        && Operations->Physical
        );

    gcmkONERROR(
        gckOS_Allocate(Os, gcmSIZEOF(gcsALLOCATOR), (gctPOINTER *)&allocator));

    gckOS_ZeroMemory(allocator, gcmSIZEOF(gcsALLOCATOR));

    /* Record os. */
    allocator->os = Os;

    /* Set operations. */
    allocator->ops = Operations;

    *Allocator = allocator;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

/***************************************************************************\
************************ Allocator management *******************************
\***************************************************************************/

gceSTATUS
gckOS_ImportAllocators(
    gckOS Os
    )
{
    gceSTATUS status;
    gctUINT i;
    gckALLOCATOR allocator;

    INIT_LIST_HEAD(&Os->allocatorList);

    for (i = 0; i < gcmCOUNTOF(allocatorArray); i++)
    {
        if (allocatorArray[i].construct)
        {
            /* Construct allocator. */
            status = allocatorArray[i].construct(Os, &allocator);

            if (gcmIS_ERROR(status))
            {
                gcmkPRINT("["DEVICE_NAME"]: Can't construct allocator(%s)",
                          allocatorArray[i].name);

                continue;
            }

            list_add(&allocator->head, &Os->allocatorList);
        }
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_FreeAllocators(
    gckOS Os
    )
{
    gckALLOCATOR allocator;
    gckALLOCATOR temp;

    list_for_each_entry_safe(allocator, temp, &Os->allocatorList, head)
    {
        list_del(&allocator->head);

        /* Free private data. */
        if (allocator->privateDataDestructor && allocator->privateData)
        {
            allocator->privateDataDestructor(allocator->privateData);
        }

        gckOS_Free(Os, allocator);
    }

    return gcvSTATUS_OK;
}

