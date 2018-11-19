/*
 * Copyright (c) 2009-2018, Christian Ferrari <tiian@users.sourceforge.net>
 * All rights reserved.
 *
 * This file is part of LIXA.
 *
 * LIXA is free software: you can redistribute this file and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * LIXA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LIXA.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"



#include <jni.h>
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_GLIB_H
# include <glib.h>
#endif



/* LIXA includes */
#include "lixa_errors.h"
#include "lixa_trace.h"
/* XTA for Java resource */
#include "xta_java_xa_resource.h"



/* set module trace flag */
#ifdef LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE   LIXA_TRACE_MOD_XTA



/**
 * Interface with XA function pointers
 */
const static struct xta_iface_s xta_java_iface = {
    "Java XAResource",
    TMNOFLAGS,
    0,
    xta_java_xa_open,
    xta_java_xa_close,
    xta_java_xa_start,
    xta_java_xa_end,
    xta_java_xa_rollback,
    xta_java_xa_prepare,
    xta_java_xa_commit,
    xta_java_xa_recover,
    xta_java_xa_forget
};



xta_java_xa_resource_t *xta_java_xa_resource_new(
    const char *name, JavaVM *java_vm, jint java_jni_version,
    jobject java_object, jmethodID start)
{
    enum Exception { G_TRY_MALLOC_ERROR
                     , XTA_JAVA_XA_RESOURCE_INIT_ERROR
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    xta_java_xa_resource_t *this = NULL;
    
    LIXA_TRACE(("xta_java_xa_resource_new\n"));
    TRY {
        /* allocate sufficient memory for the object */
        if (NULL == (this = (xta_java_xa_resource_t *)g_try_malloc0(
                         sizeof(xta_java_xa_resource_t))))
            THROW(G_TRY_MALLOC_ERROR);
        /* initialize "class" properties */
        if (LIXA_RC_OK != (ret_cod = xta_java_xa_resource_init(
                               this, name, java_vm, java_jni_version,
                               java_object, start)))
            THROW(XTA_JAVA_XA_RESOURCE_INIT_ERROR);
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case G_TRY_MALLOC_ERROR:
                ret_cod = LIXA_RC_G_TRY_MALLOC_ERROR;
                break;
            case XTA_JAVA_XA_RESOURCE_INIT_ERROR:
                break;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("xta_java_xa_resource_new/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return this;
}



void xta_java_xa_resource_delete(xta_java_xa_resource_t *xa_resource)
{
    enum Exception { NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    
    LIXA_TRACE(("xta_java_xa_resource_delete\n"));
    TRY {
        /* clean the object before releasing */
        xta_java_xa_resource_clean(xa_resource);
        /* release memory allocated for the object */
        g_free(xa_resource);
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("xta_java_xa_resource_delete/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return;
}



int xta_java_xa_resource_init(xta_java_xa_resource_t *xa_resource,
                              const char *name,
                              JavaVM *java_vm, jint java_jni_version,
                              jobject java_object, jmethodID start)
{
    enum Exception { NULL_OBJECT
                     , XTA_ACQUIRED_XA_RESOURCE_INIT_ERROR
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    
    LIXA_TRACE(("xta_java_xa_resource_init\n"));
    TRY {
        if (NULL == xa_resource)
            THROW(NULL_OBJECT);
        /* initialize "base class" (xta_acquired_xa_resource_t) properties */
        if (LIXA_RC_OK != (ret_cod = xta_acquired_xa_resource_init(
                               (xta_acquired_xa_resource_t *)xa_resource,
                               &xta_java_iface, name, "dummy")))
            THROW(XTA_ACQUIRED_XA_RESOURCE_INIT_ERROR);
        /* set references to Java XAResource */
        xa_resource->java_vm = java_vm;
        xa_resource->java_jni_version = java_jni_version;
        xa_resource->java_object = java_object;
        xa_resource->java_method_start = start;
        LIXA_TRACE(("xta_java_xa_resource_init: java_vm=%p, "
                    "java_jni_version=%d, java_object=%p, "
                    "java_method_start=%p\n",
                    xa_resource->java_vm, xa_resource->java_jni_version,
                    xa_resource->java_object, xa_resource->java_method_start));
        /* set resource interface */
        lixa_iface_set_xta(
            &xa_resource->xa_resource.act_rsrmgr_config.lixa_iface,
            &xta_java_iface, (xta_xa_resource_t *)xa_resource);
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case NULL_OBJECT:
                ret_cod = LIXA_RC_NULL_OBJECT;
                break;
            case XTA_ACQUIRED_XA_RESOURCE_INIT_ERROR:
                break;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("xta_java_xa_resource_init/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}



void xta_java_xa_resource_clean(xta_java_xa_resource_t *xa_resource)
{
    enum Exception { NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    
    LIXA_TRACE(("xta_java_xa_resource_clean\n"));
    TRY {
        /* clean Java XA function pointers */
        xa_resource->xa_resource.act_rsrmgr_config.lixa_iface.std = NULL;
        /* clean "base class" (xta_acquired_xa_resource_t) properties */
        xta_acquired_xa_resource_clean(
            (xta_acquired_xa_resource_t *)xa_resource);
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("xta_java_xa_resource_clean/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return;
}



int xta_java_xa_open(xta_xa_resource_t *context, char *xa_info,
                      int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_open: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_close(xta_xa_resource_t *context, char *xa_info,
                      int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_close: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_start(xta_xa_resource_t *context,
                      const XID *xid, int rmid, long flags)
{
    enum Exception { GET_ENV_ERROR
                     , FIND_CLASS_ERROR
                     , GET_METHOD_ID_ERROR
                     , NEW_OBJECT_ERROR
                     , XTAXID_NEWJNI
                     , CALL_VOID_METHOD_ERROR
                     , NONE } excp;
    int ret_cod = XAER_RMERR;
    
    jclass class = NULL;
    jmethodID constructor = NULL;
    jobject jxid = NULL;
    
    LIXA_TRACE(("xta_java_xa_start\n"));
    TRY {
        JNIEnv *env;
        xta_java_xa_resource_t *res = (xta_java_xa_resource_t *)context;
        LIXA_TRACE(("xta_java_xa_start: java_vm=%p, "
                    "java_jni_version=%d, java_object=%p, "
                    "java_method_start=%p\n",
                    res->java_vm, res->java_jni_version,
                    res->java_object, res->java_method_start));
        /* retrieve Java environement for this thread */
        if (JNI_OK != (*(res->java_vm))->GetEnv(
                res->java_vm, (void **)&env, res->java_jni_version))
            THROW(GET_ENV_ERROR);
        /* retrieve Java XtaXid class */
        if (NULL == (class = (*env)->FindClass(
                         env, "org/tiian/lixa/xta/XtaXid")) ||
            (*env)->ExceptionCheck(env))
            THROW(FIND_CLASS_ERROR);
        /* retrieve XtaXid constructor */
        if (NULL == (constructor = (*env)->GetMethodID(
                         env, class, "<init>", "()V")) ||
            (*env)->ExceptionCheck(env))
            THROW(GET_METHOD_ID_ERROR);
        /* create a new XtaXid object */
        if (NULL == (jxid = (*env)->NewObject(env, class, constructor)) ||
            (*env)->ExceptionCheck(env))
            THROW(NEW_OBJECT_ERROR);
        /* populate Java object with tx native C Transaction */
        Java_org_tiian_lixa_xta_XtaXid_newJNI(env, jxid, xid);
        if ((*env)->ExceptionCheck(env))
            THROW(XTAXID_NEWJNI);
        /* call Java start method using context */
        (*env)->CallVoidMethod(env, res->java_object, res->java_method_start,
                               jxid, (jint)flags);
        if ((*env)->ExceptionCheck(env))
            THROW(CALL_VOID_METHOD_ERROR);

        THROW(NONE);
    } CATCH {
        switch (excp) {
            case GET_ENV_ERROR:
            case FIND_CLASS_ERROR:
            case GET_METHOD_ID_ERROR:
            case NEW_OBJECT_ERROR:
            case XTAXID_NEWJNI:
            case CALL_VOID_METHOD_ERROR:
                break;
            case NONE:
                ret_cod = XA_OK;
                break;
            default:
                ret_cod = XAER_RMERR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("xta_java_xa_start/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}



int xta_java_xa_end(xta_xa_resource_t *context, const XID *xid,
                    int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_end: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_rollback(xta_xa_resource_t *context, const XID *xid,
                         int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_rollback: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_prepare(xta_xa_resource_t *context, const XID *xid,
                        int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_prepare: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_commit(xta_xa_resource_t *context, const XID *xid,
                       int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_commit: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_recover(xta_xa_resource_t *context,
                        XID *xids, long count, int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_recover: dummy method\n"));
    return XA_OK;
}



int xta_java_xa_forget(xta_xa_resource_t *context, const XID *xid,
                       int rmid, long flags)
{
    LIXA_TRACE(("xta_java_xa_forget: dummy method\n"));
    return XA_OK;
}

