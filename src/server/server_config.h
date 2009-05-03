/*
 * Copyright (c) 2009, Christian Ferrari
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free 
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SERVER_CONFIG_H
# define SERVER_CONFIG_H



#include <config.h>



#ifdef HAVE_LIBXML_TREE_H
# include <libxml/tree.h>
#endif
#ifdef HAVE_LIBXML_PARSER_H
# include <libxml/parser.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif



#include <server_status.h>



/* save old LIXA_TRACE_MODULE and set a new value */
#ifdef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE_SAVE LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#else
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE      LIXA_TRACE_MOD_SERVER_CONFIG



/**
 * It contains the configuration of a listener
 */
struct listener_config_s {
    /**
     * Socket domain for the listener
     */
    int domain;
    /**
     * Address used to listen by this listener
     */
    char *address;
    /**
     * Port used to listen by this listener
     */
    in_port_t port;
};

/**
 * It contains the configuration of all listeners
 */
struct listener_config_array_s {
    /**
     * Number of elements
     */
    int n;
    /**
     * Elements
     */
    struct listener_config_s *array;
};

/**
 * It contains the configuration of a manager
 */
struct manager_config_s {
    /**
     * Path of the file containing the status
     */
    char *status_file;
};

/**
 * It contains the configuration of all managers
 */
struct manager_config_array_s {
    /**
     * Number of elements
     */
    int n;
    /**
     * Elements
     */
    struct manager_config_s *array;
};

/**
 * It contains the configuration of a whole server
 */
struct server_config_s {
    /**
     * Listeners' configuration
     */
    struct listener_config_array_s      listeners;
    /**
     * Managers' configuration
     */
    struct manager_config_array_s       managers;
};



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



    /**
     * Read and parse server config file
     * @param sc OUT the object containing the server configuration
     * @param tsa IN/OUT the objects containing the status common to all
     *                   threads
     * @param config_filename IN a filename PATH must looked at before
     *                           searching default system config file
     *                           default = NULL
     * @return a standardized return code
     */
    int server_config(struct server_config_s *sc,
                      struct thread_pipe_array_s *tpa,
                      const char *config_filename);



    /**
     * Parse the configuration tree
     * @param sc OUT server configuration structure
     * @param tpa OUT thread pipe array
     * @param a_node IN the current subtree must be parsed
     * @return a standardized return code
     */
    int server_parse(struct server_config_s *sc,
                     struct thread_pipe_array_s *tpa,
                     xmlNode *a_node);

    

    /**
     * Parse a "listener" node tree
     * @param sc IN/OUT configuration structure
     * @param a_node IN listener node
     * @return a standardized return code
     */
    int server_parse_listener(struct server_config_s *sc,
                              xmlNode *a_node);
    
    
    
    /**
     * Parse a "manager" node tree
     * @param sc IN/OUT configuration structure
     * @param a_node IN listener node
     * @return a standardized return code
     */
    int server_parse_manager(struct server_config_s *sc,
                             struct thread_pipe_array_s *tpa,
                             xmlNode *a_node);
    
    
    
    /**
     * Initialize the configuration of the server
     * @param sc OUT the object must be initialized
     * @param tpa OUT the array of pipes used for thread communication
     */
    void server_config_init(struct server_config_s *sc,
                            struct thread_pipe_array_s *tpa);


    
#ifdef __cplusplus
}
#endif /* __cplusplus */



/* restore old value of LIXA_TRACE_MODULE */
#ifdef LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE_SAVE */



#endif /* SERVER_CONFIG_H */