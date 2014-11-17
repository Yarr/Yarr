/*
 * Library:   lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:      lmcurve.h
 *
 * Contents:  Declarations for Levenberg-Marquardt curve fitting.
 *
 * Copyright: Joachim Wuttke, Forschungszentrum Juelich GmbH (2004-2013)
 *
 * License:   see ../COPYING (FreeBSD)
 * 
 * Homepage:  apps.jcns.fz-juelich.de/lmfit
 */
 
#ifndef LMCURVE_H
#define LMCURVE_H
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
//#define __BEGIN_DECLS extern "C" {
//#define __END_DECLS }
#define __BEGIN_DECLS /* empty */
#define __END_DECLS /* empty */
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS /* empty */
#endif

#include<lmstruct.h>


void lmcurve( int n_par, double *par, int m_dat,
              const double *t, const double *y,
              double (*f)( double t, const double *par ),
              const lm_control_struct *control,
              lm_status_struct *status );

#endif /* LMCURVE_H */
