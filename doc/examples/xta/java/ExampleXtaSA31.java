/*
 * Copyright (c) 2009-2018, Christian Ferrari <tiian@users.sourceforge.net>
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


// import XTA package from LIXA project
import org.tiian.lixa.xta.*;

public class ExampleXtaSA31 {
    public static void main(String[] args) {
        // Check command line parameters
        if (args.length < 2) {
            System.err.println("This program requires two boolean " +
                               "parameters: 'commit' and 'insert'\n");
            System.exit(1);
        }
        // First parameter: commit transaction
        boolean commit = Integer.parseInt(args[0]) > 0 ? true : false;
        // Second parameter: insert data in databases
        boolean insert = Integer.parseInt(args[1]) > 0 ? true : false;
        // variable for PostgreSQL statement to execute
        String postgresqlStmt;
        // variable for MySQL statement to execute
        String mysqlStmt;
        /*
         * Prepare SQL statements in accordance with "insert" command line
         * parameter
         */
         if (insert) {
             postgresqlStmt = "INSERT INTO authors VALUES(1921, " +
                 "'Rigoni Stern', 'Mario')";
             mysqlStmt = "INSERT INTO authors VALUES(1919, 'Levi', 'Primo')";
         } else {
             postgresqlStmt = "DELETE FROM authors WHERE id=1921";
             mysqlStmt = "DELETE FROM authors WHERE id=1919";
         }
         try {
             TransactionManager tm = new TransactionManager();
         } catch (XtaException e) {
             System.err.println("XtaException: LIXA ReturnCode=" +
                                e.getReturnCode() + " ('" +
                                e.getMessage() + "')");
         }
         /* @@@ remove me
        if (commit)
            System.out.println("Commit");
        else
            System.out.println("Rollback");
        if (insert)
            System.out.println("Insert");
        else
            System.out.println("Delete");
        */
    }
}


/*
 * This program is an example implementation of the
 * "Single Application" Pattern
 * as documented in LIXA manual:
 * http://www.tiian.org/lixa/manuals/html/index.html
 *
 * This program accepts exactly two parameters on the command line:
 * first parameter:  "commit", boolean value (if FALSE, "rollback")
 * second parameter: "insert", boolean value (if FALSE, "delete")
 *
 * Programming Style note:
 * the purpose of this small program is not to explain C++ development
 * techniques or good style, but simply to show XTA for C++ using the easiest
 * approach.
 * "xta" namespace is explicitly put in every statement (see "xta::") but it
 * can be avoided with "using namespace xta;"
 */


/*
// Standard headers
#include <iostream>
// This header is necessary for all the stuff related to XTA
#include <xta/cpp/Xta.hpp>
*/


/*
int main(int argc, char *argv[])
{
    /* native PostgreSQL connection handler
    PGconn                       *rm1 = NULL;
    /* PostgreSQL result
    PGresult                     *pg_res;
    /* XTA Resource for PostgreSQL
    xta::PostgresqlXaResource    *xar1 = NULL;
    /* native MySQL connection handler *
    MYSQL                        *rm2 = NULL;
    /* XTA Resource for MySQL
    xta::MysqlXaResource         *xar2 = NULL;
    /* XTA Transaction Manager object reference
    xta::TransactionManager      *tm = NULL;

    /*
     * initialize XTA environment
     *
    xta::Xta::Init();
    /*
     * create a new PostgreSQL connection
     * Note: using PostgreSQL C API and C standard functions
     *
    rm1 = PQconnectdb("dbname=testdb");
    if (PQstatus(rm1) != CONNECTION_OK) {
        fprintf(stderr, "PQconnectdb: returned error %s\n",
                PQerrorMessage(rm1));
        PQfinish(rm1);
        return 1;
    }
    /*
     * create a new MySQL connection
     * Note: using MySQL C API and C standard functions
     *
    rm2 = mysql_init(NULL);
    if (rm2 == NULL) {
        fprintf(stderr, "mysql_init: returned NULL\n");
        return 1;
    }
    if (mysql_real_connect(rm2, "localhost", "lixa", "",
                           "lixa", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect: returned error: %u, %s\n",
                mysql_errno(rm2), mysql_error(rm2));
        return 1;
    }
    /*
     * create XTA objects necessary to start a transaction
     *
    try {
        // create a new XTA Transaction Manager object
        tm = new xta::TransactionManager();
        /*
         * create an XA resource for PostgreSQL
         * second parameter "PostgreSQL" is descriptive
         * third parameter "dbname=testdb" identifies the specific database
         *
        xar1 = new xta::PostgresqlXaResource(
            rm1, "PostgreSQL", "dbname=testdb");
        // create an XA resource for MySQL
        // second parameter "MySQL" is descriptive
        // third parameter "localhost,0,lixa,,lixa" identifies the specific
        // database
        xar2 = new xta::MysqlXaResource(
            rm2, "MySQL", "localhost,0,lixa,,lixa");
        // Create a new XA global transaction and retrieve a reference from
        // the TransactionManager object
        xta::Transaction tx = tm->CreateTransaction();
        // Enlist PostgreSQL resource to transaction
        tx.EnlistResource(xar1);
        // Enlist MySQL resource to Transaction
        tx.EnlistResource(xar2);
        // Open all resources enlisted by tx Transaction
        tx.Open();
        // Start a new XA global transaction with a single branch
        tx.Start();
        /*
         * At this point, it's time to do something with the Resource Managers
         * (PostgreSQL and MySQL)
         *
         * Execute PostgreSQL statement
         *
        printf("PostgreSQL, executing >%s<\n", postgresql_stmt);
        pg_res = PQexec(rm1, postgresql_stmt);
        if (PQresultStatus(pg_res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "PostgreSQL, error while executing >%s<: %s\n",
                    postgresql_stmt, PQerrorMessage(rm1));
            PQclear(pg_res);
            PQfinish(rm1);
            return 1;
        }
        PQclear(pg_res);
        /*
         * Execute MySQL statement
         *
        printf("MySQL, executing >%s<\n", mysql_stmt);
        if (mysql_query(rm2, mysql_stmt)) {
            fprintf(stderr, "MySQL, error while executing >%s<: %u/%s\n",
                    mysql_stmt, mysql_errno(rm2), mysql_error(rm2));
            mysql_close(rm2);
            return 1;
        }
        /*
         * commit or rollback the transaction
         *
        if (commit) {
            tx.Commit();
        } else {
            tx.Rollback();
        }
        /*
         * Close all resources enlisted by tx Transaction
         *
        tx.Close();
        /*
         * Delete PostgreSQL native and XA resource
         *
        delete xar1;
        /*
         * Close the PostgreSQL connection
         *
        PQfinish(rm1);
        /*
         * Delete MySQL native and XA resource
         *
        delete xar2;
        /*
         * Close the MySQL connection
         *
        mysql_close(rm2);
        /*
         * Delete Transaction Manager object
         *
        delete tm;
        
    } catch (xta::Exception e) {
        /*
         * what() is a standard method that describes the exception
         * where() is a method provided by XTA to describe the XTA C function
         *         that raised the exception
         * getReturnCode() is a method provided by XTA to retrieve the
         *                 integer reason code returned by XTA C function
         *                 (see file lixa_errors.h)
         *
        cerr << "Exception in function '" << e.where() <<
            "', return code description: '" << e.what() << "', " <<
            "return code: " << e.getReturnCode() << endl;
        return 1;
    }
    
    return 0;
}
*/
