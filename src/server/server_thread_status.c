/*
 * Copyright (c) 2009-2010, Christian Ferrari <tiian@users.sourceforge.net>
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
#include <config.h>



#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif



#include <lixa_errors.h>
#include <lixa_trace.h>
#include <server_thread_status.h>



/* set module trace flag */
#ifdef LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE   LIXA_TRACE_MOD_SERVER_STATUS



void thread_status_init(struct thread_status_s *ts,
                        int id,
                        struct thread_pipe_array_s *tpa)
{
    LIXA_TRACE(("thread_status_init: initializing thread status (id = %d)\n",
                id));
    ts->id = id;
    ts->tpa = tpa;
    ts->poll_size = 0;
    ts->poll_array = NULL;
    ts->active_clients = 0;
    ts->client_array = NULL;
    ts->asked_sync = 0;
    ts->status1_filename = ts->status2_filename = NULL;
    ts->status1 = ts->status2 = NULL;
    ts->curr_status = NULL;
    ts->updated_records = g_tree_new(size_t_compare_func);
    ts->recovery_table = NULL;
    ts->excp = ts->ret_cod = ts->last_errno = 0;
    if (id == 0)
        ts->tid = pthread_self();
    else
        ts->tid = 0;
    LIXA_TRACE(("thread_status_init: end initialization (id = %d)\n", id));
}



int thread_status_load_files(struct thread_status_s *ts,
                             const char *status_file_prefix)
{
    enum Exception { STATUS_RECORD_LOAD_1_ERROR
                     , STATUS_RECORD_LOAD_2_ERROR
                     , DAMAGED_STATUS_FILES
                     , STATUS_RECORD_COPY_ERROR1
                     , STATUS_RECORD_COPY_ERROR2
                     , STATUS_RECORD_COPY_ERROR3
                     , STATUS_RECORD_COPY_ERROR4
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    
    LIXA_TRACE(("thread_status_load_files\n"));
    TRY {
        int s1ii = FALSE, s2ii = FALSE;
        
        /* first file */
        ts->status1_filename = g_strconcat(status_file_prefix,
                                           STATUS_FILE_SUFFIX_1, NULL);
        LIXA_TRACE(("thread_status_load_files: first status file is '%s'\n",
                    ts->status1_filename));
        if (LIXA_RC_OK != (ret_cod = status_record_load(
                               &(ts->status1),
                               (const char *)ts->status1_filename,
                               ts->updated_records)))
            THROW(STATUS_RECORD_LOAD_1_ERROR);
        if (LIXA_RC_OK != (ret_cod = status_record_check_integrity(
                               ts->status1)))
            syslog(LOG_WARNING, "thread_status_load_files: first status file "
                   "('%s') did not pass integrity check\n",
                   ts->status1_filename);
        else
            s1ii = TRUE;
        
        /* second file */
        ts->status2_filename = g_strconcat(status_file_prefix,
                                           STATUS_FILE_SUFFIX_2, NULL);
        LIXA_TRACE(("thread_status_load_files: second status file is '%s'\n",
                    ts->status2_filename));
        if (LIXA_RC_OK != (ret_cod = status_record_load(
                               &(ts->status2),
                               (const char *)ts->status2_filename,
                               ts->updated_records)))
            THROW(STATUS_RECORD_LOAD_2_ERROR);
        if (LIXA_RC_OK != (ret_cod = status_record_check_integrity(
                               ts->status2)))
            syslog(LOG_WARNING, "thread_status_load_files: second status file "
                   "('%s') did not pass integrity check\n",
                   ts->status2_filename);
        else
            s2ii = TRUE;

        if (!s1ii && !s2ii) {
            /* two damaged files! */
            syslog(LOG_ERR, "thread_status_load_files: both status files "
                   "did not pass integrity check; the server can not "
                   "start-up");
            THROW(DAMAGED_STATUS_FILES);
        } else if (s1ii && s2ii) {
            /* two integral files, check timestamp */
            if ((ts->status1->sr.ctrl.last_sync.tv_sec ==
                 ts->status2->sr.ctrl.last_sync.tv_sec) &&
                (ts->status1->sr.ctrl.last_sync.tv_usec ==
                 ts->status2->sr.ctrl.last_sync.tv_usec)) {
                LIXA_TRACE(("thread_status_load_files: first and second "
                            "status file were synchronized at the same "
                            "time\n"));
                ts->curr_status = ts->status1;
            } else if ((ts->status1->sr.ctrl.last_sync.tv_sec <
                 ts->status2->sr.ctrl.last_sync.tv_sec) ||
                ((ts->status1->sr.ctrl.last_sync.tv_sec ==
                  ts->status2->sr.ctrl.last_sync.tv_sec) &&
                 (ts->status1->sr.ctrl.last_sync.tv_usec <
                  ts->status2->sr.ctrl.last_sync.tv_usec))) {
                /* second file is newer */
                LIXA_TRACE(("thread_status_load_files: second status file is "
                            "the more recent\n"));
                /* copying second file over first one, and point first as
                   the current file */
                if (LIXA_RC_OK != (ret_cod = status_record_copy(
                                       ts->status1, ts->status2, ts)))
                    THROW(STATUS_RECORD_COPY_ERROR1);
                ts->curr_status = ts->status1;
            } else {
                /* first file is newer */
                LIXA_TRACE(("thread_status_load_files: first status file is "
                            "the more recent\n"));
                /* copying first file over second one, and point second as
                   the current file */
                if (LIXA_RC_OK != (ret_cod = status_record_copy(
                                       ts->status2, ts->status1, ts)))
                    THROW(STATUS_RECORD_COPY_ERROR2);
                ts->curr_status = ts->status2;
            }
        } else if (s1ii) {
            /* only first file is integral */
            LIXA_TRACE(("thread_status_load_files: first status file is OK, "
                        "second status file is damanged: oveeriding it...\n"));
            /* copying first file over second one, and point second as
               the current file */
            if (LIXA_RC_OK != (ret_cod = status_record_copy(
                                   ts->status2, ts->status1, ts)))
                THROW(STATUS_RECORD_COPY_ERROR3);
            ts->curr_status = ts->status2;
        } else {
            /* only second file is integral */
            LIXA_TRACE(("thread_status_load_files: second status file is OK, "
                        "first status file is damanged: overriding it...\n"));
            /* copying second file over first one, and point first as
               the current file */
            if (LIXA_RC_OK != (ret_cod = status_record_copy(
                                   ts->status1, ts->status2, ts)))
                THROW(STATUS_RECORD_COPY_ERROR4);
            ts->curr_status = ts->status1;
        }
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case STATUS_RECORD_LOAD_1_ERROR:
            case STATUS_RECORD_LOAD_2_ERROR:
                break;
            case DAMAGED_STATUS_FILES:
                ret_cod = LIXA_RC_CORRUPTED_STATUS_FILE;
                break;
            case STATUS_RECORD_COPY_ERROR1:
            case STATUS_RECORD_COPY_ERROR2:
            case STATUS_RECORD_COPY_ERROR3:
            case STATUS_RECORD_COPY_ERROR4:
                break;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("thread_status_load_files/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}


    
int thread_status_recovery(struct thread_status_s *ts,
                           srvr_rcvr_tbl_t *srt)
{
    enum Exception { NULL_SERVER_RECOVERY_TABLE
                     , CHECK_RECOVERY_PENDING_ERROR
                     , RECOVERY_TABLE_INSERT_ERROR
                     , INVALID_STATUS
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;
    
    LIXA_TRACE(("thread_status_recovery\n"));
    TRY {
        uint32_t i;
        struct status_record_s *first_block = ts->curr_status;
        
        if (NULL == srt)
            THROW(NULL_SERVER_RECOVERY_TABLE);
        ts->recovery_table = srt;
        
        /* traverse used block list */
        i = first_block->sr.ctrl.first_used_block;
        while (0 != i) {
            struct status_record_data_s *data = &ts->curr_status[i].sr.data;
            if (DATA_PAYLOAD_TYPE_RSRMGR == data->pld.type) {
                LIXA_TRACE(("thread_status_recovery: block # " UINT32_T_FORMAT
                            " is a transaction resource manager block, "
                            "skipping...\n", i));
            } else if (DATA_PAYLOAD_TYPE_HEADER == data->pld.type) {
                int recovery_pending = FALSE;
                LIXA_TRACE(("thread_status_recovery: block # " UINT32_T_FORMAT
                            " is a transaction header block\n", i));
                if (LIXA_RC_OK != (
                        ret_cod = thread_status_check_recovery_pending(
                            data, &recovery_pending)))
                    THROW(CHECK_RECOVERY_PENDING_ERROR);
                if (recovery_pending) {
                    struct srvr_rcvr_tbl_rec_s srtr;
                    LIXA_TRACE(("thread_status_recovery: block # "
                                UINT32_T_FORMAT " is related to a recovery "
                                "pending transaction\n", i));
                    srtr.job = &data->pld.ph.job;
                    srtr.tsid = ts->id;
                    srtr.block_id = i;
                    if (LIXA_RC_OK != (ret_cod = srvr_rcvr_tbl_insert(
                                           ts->recovery_table, &srtr)))
                        THROW(RECOVERY_TABLE_INSERT_ERROR);
                }
            } else {
                LIXA_TRACE(("thread_status_recovery: block # " UINT32_T_FORMAT
                            " is an unknown block (%d)\n", i, data->pld.type));
                THROW(INVALID_STATUS);
            }
            i = data->next_block;
        } /* for (i...) */
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case NULL_SERVER_RECOVERY_TABLE:
                ret_cod = LIXA_RC_NULL_OBJECT;
                break;
            case CHECK_RECOVERY_PENDING_ERROR:
            case RECOVERY_TABLE_INSERT_ERROR:
                break;
            case INVALID_STATUS:
                ret_cod = LIXA_RC_INVALID_STATUS;
                break;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("thread_status_recovery/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}



int thread_status_check_recovery_pending(
    const struct status_record_data_s *data, int *result)
{
    enum Exception { INVALID_HEADER_TYPE
                     , FINISHED_TRANSACTION
                     , NOT_STARTED_TRANSACTION
                     , INVALID_VERB
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    /* conservative behavior */
    *result = TRUE;
    
    LIXA_TRACE(("thread_status_check_recovery_pending\n"));
    TRY {
        const struct lixa_msg_verb_step_s *last = data->pld.ph.last_verb_step;
        
        if (DATA_PAYLOAD_TYPE_HEADER != data->pld.type) {
            LIXA_TRACE(("thread_status_check_recovery_pending: "
                        "data->pld.type=%d\n", data->pld.type));
            THROW(INVALID_HEADER_TYPE);
        }

        /* the logic of this function could be improved in the future, but
           at this time the algorithm is very conservative: probably some
           unnecessary recovery operations will be performed */
        
        /* is the transaction already marked as finished? */
        if (data->pld.ph.state.finished) {
            LIXA_TRACE(("thread_status_check_recovery_pending: "
                        "data->pld.ph.state.finished=%d returning FALSE\n",
                        data->pld.ph.state.finished));
            THROW(FINISHED_TRANSACTION);
        }
        /* check last verb & step */
        switch (last->verb) {
            case LIXA_MSG_VERB_OPEN:
            case LIXA_MSG_VERB_CLOSE:
                LIXA_TRACE(("thread_status_check_recovery_pending: "
                            "last->verb=%d returning FALSE\n", last->verb));
                THROW(NOT_STARTED_TRANSACTION);
            case LIXA_MSG_VERB_START:
            case LIXA_MSG_VERB_END:
            case LIXA_MSG_VERB_PREPARE:
            case LIXA_MSG_VERB_COMMIT:
            case LIXA_MSG_VERB_ROLLBACK:
                break;
            default:
                THROW(INVALID_VERB);
        }
        /* arrived here = possibly recovery pending... */
        
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case INVALID_HEADER_TYPE:
            case INVALID_VERB:
                ret_cod = LIXA_RC_INVALID_STATUS;
                break;
            case FINISHED_TRANSACTION:
            case NOT_STARTED_TRANSACTION:
                *result = FALSE;
                ret_cod = LIXA_RC_OK;
                break;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
    } /* TRY-CATCH */
    LIXA_TRACE(("thread_status_check_recovery_pending/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}


    
int thread_status_sync_files(struct thread_status_s *ts)
{
    enum Exception { GETTIMEOFDAY_ERROR
                     , STATUS_RECORD_CHECK_INTEGRITY_ERROR
                     , MSYNC_ERROR
                     , MUNMAP_ERROR
                     , TRUNCATE_ERROR
                     , OPEN_ERROR
                     , MMAP_ERROR
                     , CLOSE_ERROR
                     , MEMCMP_ERROR
                     , NONE } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    int alt_fd = LIXA_NULL_FD;
    
    LIXA_TRACE(("thread_status_sync_files\n"));
    TRY {
        status_record_t *alt_status;
        status_record_t **status_is;
        gchar *alt_filename;
        struct two_status_record_s tsr;
        off_t curr_status_size = 0;
        
        if (LIXA_RC_OK != (ret_cod = gettimeofday(
                               &ts->curr_status->sr.ctrl.last_sync, NULL)))
            THROW(GETTIMEOFDAY_ERROR);
        status_record_update(ts->curr_status, 0, ts->updated_records);
        g_tree_foreach(ts->updated_records, traverse_and_sync,
                       ts->curr_status);
#ifdef LIXA_DEBUG
        if (LIXA_RC_OK != (ret_cod = status_record_check_integrity(
                               ts->curr_status)))
            THROW(STATUS_RECORD_CHECK_INTEGRITY_ERROR);
#endif /* LIXA_DEBUG */
        LIXA_TRACE(("thread_status_sync_files: before msync\n"));
        if (-1 == msync(ts->curr_status,
                        ts->curr_status->sr.ctrl.number_of_blocks *
                        sizeof(status_record_t), MS_SYNC))
            THROW(MSYNC_ERROR);
        LIXA_TRACE(("thread_status_sync_files: after first msync\n"));
        /* current status file can become the baseline, discover and process
           alternate status file */
        if (ts->curr_status == ts->status1) {
            alt_status = ts->status2;
            status_is = &(ts->status2);
            alt_filename = ts->status2_filename;
        } else {
            alt_status = ts->status1;
            status_is = &(ts->status1);
            alt_filename = ts->status1_filename;
        }

        curr_status_size = sizeof(status_record_t) *
            ts->curr_status->sr.ctrl.number_of_blocks;
        
        /* compare size */
        if (ts->curr_status->sr.ctrl.number_of_blocks >
            alt_status->sr.ctrl.number_of_blocks) {
            off_t alt_status_size = sizeof(status_record_t) *
                alt_status->sr.ctrl.number_of_blocks;
            /* elarge alternate status file */
            LIXA_TRACE(("thread_status_sync_files: current status file "
                        "contains " UINT32_T_FORMAT
                        " blocks while alternate status file contains "
                        UINT32_T_FORMAT " blocks; I must perform file "
                        "enlargment before content copy\n",
                        ts->curr_status->sr.ctrl.number_of_blocks,
                        alt_status->sr.ctrl.number_of_blocks));
            /* reset the pointer in thread status structure... */
            *status_is = NULL;
            if (0 != munmap(alt_status, alt_status_size))
                THROW(MUNMAP_ERROR);
            if (-1 == truncate((const char *)alt_filename,
                               curr_status_size))
                THROW(TRUNCATE_ERROR);
            if (-1 == (alt_fd = open((const char *)alt_filename, O_RDWR)))
                THROW(OPEN_ERROR);
            if (NULL == (alt_status = mmap(NULL, curr_status_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, alt_fd, 0)))
                THROW(MMAP_ERROR);
            if (0 != close(alt_fd)) {
                alt_fd = LIXA_NULL_FD;
                THROW(CLOSE_ERROR);
            }
            alt_fd = LIXA_NULL_FD;
        }
        /* copy modified records */
        tsr.first = ts->curr_status;
        tsr.second = alt_status;
        g_tree_foreach(ts->updated_records, traverse_and_copy, &tsr);
#ifdef LIXA_DEBUG
        /* the memory mapped status file must be equal */
        if (0 != memcmp(ts->curr_status, alt_status, curr_status_size)) {
            LIXA_TRACE(("thread_status_sync_files: memory mapped status "
                        "files are different after copy. INTERNAL ERROR\n"));
            syslog(LOG_ERR, "thread_status_sync_files: memory mapped status "
                   "files are different after copy. INTERNAL ERROR");
            THROW(MEMCMP_ERROR);
        }
#endif /* LIXA_DEBUG */
        /* clean updated records set */
        thread_status_updated_records_clean(ts->updated_records);
        /* recover the pointer in thread status structure... */
        *status_is = alt_status;
        /* switch to alternate status mapped file */
        LIXA_TRACE(("thread_status_sync_files: switching current memory "
                    "mapped status file from %p...\n", ts->curr_status));
        ts->curr_status = alt_status;
        LIXA_TRACE(("thread_status_sync_files: ... switching current memory "
                    "mapped status file to %p...\n", ts->curr_status));
        THROW(NONE);
    } CATCH {
        switch (excp) {
            case GETTIMEOFDAY_ERROR:
                ret_cod = LIXA_RC_GETTIMEOFDAY_ERROR;
                break;
            case STATUS_RECORD_CHECK_INTEGRITY_ERROR:
                break;
            case MSYNC_ERROR:
                ret_cod = LIXA_RC_MSYNC_ERROR;
                break;
            case MUNMAP_ERROR:
                ret_cod = LIXA_RC_MUNMAP_ERROR;
                break;
            case TRUNCATE_ERROR:
                ret_cod = LIXA_RC_TRUNCATE_ERROR;
                break;
            case OPEN_ERROR:
                ret_cod = LIXA_RC_OPEN_ERROR;
                break;
            case MMAP_ERROR:
                ret_cod = LIXA_RC_MMAP_ERROR;
                break;
            case CLOSE_ERROR:
                ret_cod = LIXA_RC_CLOSE_ERROR;
                break;
            case MEMCMP_ERROR:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
            case NONE:
                ret_cod = LIXA_RC_OK;
                break;
            default:
                ret_cod = LIXA_RC_INTERNAL_ERROR;
        } /* switch (excp) */
        /* recovery actions */
        if (LIXA_NULL_FD  != alt_fd) {
            LIXA_TRACE(("thread_status_sync_files: values before recovery "
                        "actions excp=%d/ret_cod=%d/errno=%d\n",
                        excp, ret_cod, errno));
            if (LIXA_NULL_FD != alt_fd) {
                LIXA_TRACE(("thread_status_sync_files: closing file "
                            "descriptor %d\n", alt_fd));
                close(alt_fd);
            }
        }
    } /* TRY-CATCH */
    LIXA_TRACE(("thread_status_sync_files/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}

        