/*
 * Copyright (c) 2009-2019, Christian Ferrari <tiian@users.sourceforge.net>
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
 */
#ifndef LIXA_STATE_H
# define LIXA_STATE_H



#include "config.h"



#ifdef HAVE_GLIB_H
# include <glib.h>
#endif



#include "lixa_trace.h"
#include "lixa_state_file.h"
#include "lixa_state_log.h"



/* save old LIXA_TRACE_MODULE and set a new value */
#ifdef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE_SAVE LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#else
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE      LIXA_TRACE_MOD_SERVER_STATUS



/**
 * Number of state files and log files; can NOT be less than 3
 */
#define LIXA_STATE_FILES   3



/**
 * Suffix appended when a new state file is created
 */
extern const char *LIXA_STATE_FILE_SUFFIX;
/**
 * Suffix appended when a new log file is created
 */
extern const char *LIXA_STATE_LOG_SUFFIX;



/**
 * LIXA State data type ("class")
 */
typedef struct lixa_state_s {
    /**
     * System page file
     */
    size_t                system_page_size;
    /**
     * Maximum number of log records before a flush happens; it must be > 0
     */
    uint32_t              flush_max_log_records;
    /**
     * Points to the state file (and state log) currently in use
     */
    int                   used_state_file;
    /**
     * Array of state files
     */
    lixa_state_file_t     files[LIXA_STATE_FILES];
    /**
     * Array of state logs
     */
    lixa_state_log_t      logs[LIXA_STATE_FILES];
} lixa_state_t;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    
    /**
     * Initialize a State object to manage state server persistence
     * @param[in,out] this object to be initialized
     * @param[in] path_prefix of the state and log files
     * @param[in] flush_max_log_records is the maximum number of log records
     *            to be buffered before a flush happens; 0 = unlimited
     * @return a reason code
     */
    int lixa_state_init(lixa_state_t *this, const char *path_prefix,
                        uint32_t flush_max_log_records);



    /**
     * Analyze the available state and log files: establish if a warm restart
     * or a cold start must be performed
     * @param[in,out] this state object
     * @param[in] state_exists specifies which state files are available
     * @param[in] log_exists specifies which log files are available
     * @param[out] cold_start returns TRUE if a cold start must be performed
     * @return a reason code
     */     
    int lixa_state_analyze(lixa_state_t *this,
                           const int *state_exists,
                           const int *log_exists,
                           int *cold_start);

    
    
    /**
     * Create new state and log files for this state object because a cold
     * start must be done
     * @param[in,out] this state object
     * @return a reason code
     */
    int lixa_state_cold_start(lixa_state_t *this);

    
    
    /**
     * Cleanup a State object
     * @param[in,out] this state object
     * @return a reason code
     */
    int lixa_state_clean(lixa_state_t *this);



    
    /**
     * Return a boolean value that is true when the buffer of the state log
     * must be flushed
     * @param[in] this state object
     * @return a boolean value
     */
    int lixa_state_buffer_must_be_flushed(lixa_state_t *this);
    


    /**
     * Flush the records to the state log
     * @param[in,out] this state object
     * @param[in] status_records @@@ move to lixa_state_file_t?
     * @return a reason code
     */
    int lixa_state_flush_log_records(lixa_state_t *this,
                                     status_record_t *status_records);

    

    /**
     * Mark a block because it has been updated
     * @param[in,out] this state object
     * @param[in] block_id of the changed block
     * @return a reason code
     */
    int lixa_state_mark_block(lixa_state_t *this, uint32_t block_id);


    
#ifdef __cplusplus
}
#endif /* __cplusplus */



/* restore old value of LIXA_TRACE_MODULE */
#ifdef LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE_SAVE */



#endif /* LIXA_STATE_H */
