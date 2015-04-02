/* Copyright (c) 2004, David Leonard. All rights reserved. */
/* $Id: init.h 1032 2006-02-19 13:59:17Z d $ */

#ifndef _SEE_h_init
#define _SEE_h_init

/*
 * Initialisers and allocators used by the interpreter initialisation
 * code (SEE_interpreter_init) are declared here in one place.
 */

/* obj_Array.c */
void SEE_Array_alloc(struct SEE_interpreter *);
void SEE_Array_init(struct SEE_interpreter *);

/* obj_Boolean.c */
void SEE_Boolean_alloc(struct SEE_interpreter *);
void SEE_Boolean_init(struct SEE_interpreter *);

/* obj_Date.c */
void SEE_Date_alloc(struct SEE_interpreter *);
void SEE_Date_init(struct SEE_interpreter *);

/* obj_Error.c */
void SEE_Error_alloc(struct SEE_interpreter *);
void SEE_Error_init(struct SEE_interpreter *);  

/* obj_Function.c */  
void SEE_Function_alloc(struct SEE_interpreter *);
void SEE_Function_init(struct SEE_interpreter *);

/* obj_Global.c */
void SEE_Global_alloc(struct SEE_interpreter *);
void SEE_Global_init(struct SEE_interpreter *);

/* obj_Math.c */
void SEE_Math_alloc(struct SEE_interpreter *);
void SEE_Math_init(struct SEE_interpreter *);

/* obj_Number.c */
void SEE_Number_alloc(struct SEE_interpreter *);
void SEE_Number_init(struct SEE_interpreter *);

/* obj_Object.c */
void SEE_Object_alloc(struct SEE_interpreter *);
void SEE_Object_init(struct SEE_interpreter *);

/* obj_RegExp.c */
void SEE_RegExp_alloc(struct SEE_interpreter *);
void SEE_RegExp_init(struct SEE_interpreter *);

/* obj_String.c */
void SEE_String_alloc(struct SEE_interpreter *);
void SEE_String_init(struct SEE_interpreter *);

/* module.c */
void _SEE_module_alloc(struct SEE_interpreter *);
void _SEE_module_init(struct SEE_interpreter *);
void _SEE_module_fini(struct SEE_interpreter *);

#endif /* _SEE_h_init */
