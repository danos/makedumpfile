/*
 * mips64.c
 *
 * Copyright (C) 2021 Loongson Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef __mips64__

#include "../print_info.h"
#include "../elf_info.h"
#include "../makedumpfile.h"

int
get_phys_base_mips64(void)
{
	info->phys_base = 0ULL;

	DEBUG_MSG("phys_base    : %lx\n", info->phys_base);

	return TRUE;
}

int
get_machdep_info_mips64(void)
{
	info->max_physmem_bits  = _MAX_PHYSMEM_BITS;
	info->section_size_bits = _SECTION_SIZE_BITS;

	DEBUG_MSG("max_physmem_bits : %lx\n", info->max_physmem_bits);
	DEBUG_MSG("section_size_bits: %lx\n", info->section_size_bits);

	return TRUE;
}

int
get_versiondep_info_mips64(void)
{
	info->page_offset  = 0x9800000000000000ULL;

	DEBUG_MSG("page_offset : %lx\n", info->page_offset);

	return TRUE;
}

/*
 * Translate a virtual address to a physical address by using 3 levels paging.
 */
unsigned long long
vaddr_to_paddr_mips64(unsigned long vaddr)
{
	unsigned long long paddr = NOT_PADDR;
	pgd_t	*pgda, pgdv;
	pmd_t	*pmda, pmdv;
	pte_t 	*ptea, ptev;

	/*
	 * CKSEG0/CKSEG1
	 */
	if (vaddr >= 0xffffffff80000000ULL && vaddr < 0xffffffffc0000000ULL)
		return vaddr & 0x1fffffffULL;

	/*
	 * XKPHYS
	 */
	if (vaddr >= 0x9000000000000000ULL && vaddr < 0xc000000000000000ULL)
		return vaddr & ((1ULL << MAX_PHYSMEM_BITS()) - 1);

	if (SYMBOL(swapper_pg_dir) == NOT_FOUND_SYMBOL) {
		ERRMSG("Can't get the symbol of swapper_pg_dir.\n");
		return NOT_PADDR;
	}

	pgda = pgd_offset(SYMBOL(swapper_pg_dir), vaddr);
	if (!readmem(PADDR, (unsigned long long)pgda, &pgdv, sizeof(pgdv))) {
		ERRMSG("Can't read pgd\n");
		return NOT_PADDR;
	}

	pmda = pmd_offset(&pgdv, vaddr);
	if (!readmem(PADDR, (unsigned long long)pmda, &pmdv, sizeof(pmdv))) {
		ERRMSG("Can't read pmd\n");
		return NOT_PADDR;
	}

	switch (pmdv & (_PAGE_PRESENT|_PAGE_HUGE)) {
	case _PAGE_PRESENT:
		ptea = pte_offset(&pmdv, vaddr);
		/* 64k page */
		if (!readmem(PADDR, (unsigned long long)ptea, &ptev, sizeof(ptev))) {
			ERRMSG("Can't read pte\n");
			return NOT_PADDR;
		}

		if (!(ptev & _PAGE_PRESENT)) {
			ERRMSG("Can't get a valid pte.\n");
			return NOT_PADDR;
		} else {
			paddr = PAGEBASE(ptev) + (vaddr & (PAGESIZE() - 1));
		}
		break;
	case _PAGE_PRESENT|_PAGE_HUGE:
		paddr = (pmdv & PMD_MASK) + (vaddr & (PMD_SIZE - 1));
		break;
	}

	return paddr;
}

#endif /* mips64 */
