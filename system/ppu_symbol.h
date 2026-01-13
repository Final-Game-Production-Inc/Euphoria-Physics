//
// system/ppu_symbol.h
//
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_PPU_SYMBOL_H
#define SYSTEM_PPU_SYMBOL_H

// DECLARE_PPU_SYMBOL declares a symbol that lives on the main processor.
// PPU_SYMBOL is a macro that resolves to that object's address on the main processor.

#if __SPU

#define DECLARE_PPU_SYMBOL(type,ppu_sym) \
	extern char _cell_spurs_##ppu_sym[] __asm__ (#ppu_sym "@ppu"); \
	type *ppu_sym##_ptr = (type*) _cell_spurs_##ppu_sym

#define PPU_SYMBOL(ppu_sym)	((__typeof(ppu_sym##_ptr))(unsigned int)* &ppu_sym##_ptr)

#else

#define DECLARE_PPU_SYMBOL(type,ppu_sym)	extern type ppu_sym

#define PPU_SYMBOL(ppu_sym)					&ppu_sym

#endif

#endif // SYSTEM_PPU_SYMBOL_H
