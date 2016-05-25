/*
 * Copyright (c) 2009-2016, Christian Ferrari <tiian@users.sourceforge.net>
 * All rights reserved.
 *
 * This file is part of LIXA.
 *
 * LIXA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * LIXA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LIXA.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file is part of the libraries provided by LIXA.
 * In addition, as a special exception, the copyright holders of LIXA gives
 * Globetom Holdings (Pty) Ltd / 92 Regency Drive / Route 21 Corporate Park /
 * Nellmapius Drive / Centurion / South Africa
 * the permission to redistribute this file and/or modify it under the terms
 * of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 * The above special grant is perpetual and restricted to
 * Globetom Holdings (Pty) Ltd: IN NO WAY it can be automatically transferred
 * to a different company or to different people. "In no way" contemplates:
 * merge, acquisition and any other possible form of corportate change.
 * IN NO WAY, the above special grant can be assimilated to a patent or
 * any other form of asset.
 */
#ifndef LIXA_XID_H
# define LIXA_XID_H



#include <config.h>



#ifdef HAVE_UUID_H
# include <uuid.h>
#endif
#ifdef HAVE_UUID_UUID_H
# include <uuid/uuid.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif



#include <xa.h>
#include <lixa_trace.h>



/* save old LIXA_TRACE_MODULE and set a new value */
#ifdef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE_SAVE LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#else
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE      LIXA_TRACE_MOD_COMMON_XID



/**
 * This is the formatID associated to XID generated by LIXA
 * 32 bytes for gtrid (Global TRansaction ID)
 * 16 bytes for bqual (Branch QUALifier)
 */
extern const long LIXA_XID_FORMAT_ID;



/**
 * Character separator used when serializing/deserializing a xid
 */
#define LIXA_XID_SEPARATOR   '.'



/**
 * Length of a string that can contain a serialized XID:
 * formatID + separator + gtrid + separator + bqual + terminator
 */
#define LIXA_XID_SERIALIZE_LENGTH (LIXA_SERIALIZED_LONG_INT+1+2*XIDDATASIZE+1+1)



/**
 * A string used to serialize a XID.
 * NOTE: this is not XA standard compliant, but it just works in
 * conjunction with LIXA Transaction Manager.
 */
typedef char lixa_ser_xid_t[LIXA_XID_SERIALIZE_LENGTH];



/**
 * Global branch qualifier: it's unique for every thread of a process;
 * every process acts as a distinct transaction manager
 */
extern uuid_t lixa_xid_global_bqual;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



    /**
     * Use an MD5 digest to set the global bqual
     * <b>Note:</b> this function is not thread safe and MUST be called
     * with a serialization technique
     * @param md5_digest_hex IN pointer to a string of
     * @ref MD5_DIGEST_LENGTH * 2 characters
     */
    void lixa_xid_set_global_bqual(const char *md5_digest_hex);



    /**
     * Check if the branch qualifier of the transaction matched the global
     * branch qualifier of current running transaction manager instance
     * @param xid IN transaction id to inspect
     * @return a boolean value
     */
    int lixa_xid_bqual_is_global(const XID *xid);

    
                            
    /**
     * Create a new XID
     * @param xid OUT the generated unique transaction id
     */
    void lixa_xid_create_new(XID *xid);



    /**
     * Retrieve an ASCII string with the human readable version of the gtrid
     * (Global TRansaction ID).
     * @param xid IN unique transaction id
     * @return a string MUST be freed by the caller using "free" function or
     *         NULL if an error happens
     */
    char *lixa_xid_get_gtrid_ascii(const XID *xid);


    
    /**
     * Retrieve an ASCII string with the human readable version of the bqual
     * (Branch QUALifier).
     * @param xid IN unique transaction id
     * @return a string MUST be freed by the caller using "free" function or
     *         NULL if an error happens
     */
    char *lixa_xid_get_bqual_ascii(const XID *xid);



    /**
     * Reset a xid structure
     * @param xid IN/OUT transaction id to be resetted
     */
    static inline void lixa_xid_reset(XID *xid) {
        memset(xid, 0, sizeof(XID));
        xid->formatID = NULLXID;
    }


    
    /**
     * Retrieve the LIXA format ID serialized; it's useful to query PostgreSQL
     * and retrieve all the current prepared transactions (xa_recover function)
     * @param lsx OUT the serialized format ID
     */
    void lixa_xid_formatid(lixa_ser_xid_t lsx);



    /**
     * Serialize XID to a string
     * @param xid IN the XID to be serialized
     * @param lsx OUT the serialized XID
     * @return TRUE if serialization was completed, FALSE if there was an error
     */
    int lixa_xid_serialize(const XID *xid, lixa_ser_xid_t lsx);



    /**
     * Deserialize a string compatible with PostgreSQL to a XID
     * @param xid OUT the deserialized XID
     * @param lsx IN the string must be deserialized
     * @return TRUE if deserialization was completed,
     *         FALSE if there was an error
     */
    int lixa_xid_deserialize(XID *xid, lixa_ser_xid_t lsx);



    /**
     * Compare two xids
     * @param a IN first object to compare
     * @param b IN second object to compare
     * @return if (a==b) --> 0 <br>
     *         if (a<b) --> -1 <br>
     *         if (a>b) --> +1
     */
    int lixa_xid_compare(const XID *a, const XID *b);



#ifdef __cplusplus
}
#endif /* __cplusplus */



/* restore old value of LIXA_TRACE_MODULE */
#ifdef LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE_SAVE */



#endif /* LIXA_XID_H */
