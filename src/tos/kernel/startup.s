/* Copyright 2024 Grug Huhler.  License SPDX BSD-2-Clause.

   This is a VERY incomplete startup sequence for C.  It fails to
   do many things such as clear bss.
*/

.text
.global _start
_start:
	lui x2, 0x40800  /* 0x40000000 + 8 * 1024 * 1024 */
	call kernel_main
