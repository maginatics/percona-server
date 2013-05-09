.. _slow_extended_55:

================
 Slow Query Log
================

This feature adds microsecond time resolution and additional statistics to the slow query log output. It lets you enable or disable the slow query log at runtime, adds logging for the slave SQL thread, and adds fine-grained control over what and how much to log into the slow query log.

The ability to log queries with microsecond precision is essential for measuring the work the |MySQL| server performs. The standard slow query log in |MySQL| 5.0 has only 1-second granularity, which is too coarse for all but the slowest queries. |MySQL| 5.1 has microsecond resolution, but does not have the extra information about query execution that is included in the |Percona Server|.

You can use *Percona-Toolkit*'s `pt-query-digest <http://www.percona.com/doc/percona-toolkit/2.1/pt-query-digest.html>`_ tool to aggregate similar queries together and report on those that consume the most execution time.


Version Specific Information
============================

  * 5.5.8-20.0:
    Added values ``profiling`` and ``profiling_use_getrusage`` to variable log_slow_verbosity.

  * 5.5.10-20.1:
     * Renamed variable :variable:`slow_query_log_timestamp_always` to :variable:`slow_query_log_timestamp_always`.

     * Renamed variable :variable:`slow_query_log_microseconds_timestamp` to :variable:`slow_query_log_timestamp_precision`.

     * Renamed variable :variable:`use_global_log_slow_control` to :variable:`slow_query_log_use_global_control`.

Other Information
=================

  * Author / Origin:
    Maciej Dobrzanski, Percona

System Variables
================

.. variable:: log_slow_admin_statements

     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: yes

When this variable is enabled, administrative statements will be logged to the slow query log. Upstream version of the |MySQL| server has implemented command line option with same name. Significant difference is that this feature is implemented as variable in |Percona Server|, that means it can be enabled/disabled dynamically without restarting the server.

.. variable:: log_slow_filter

     :cli: Yes
     :conf: Yes
     :scope: Global, Session
     :dyn: Yes

Filters the slow log by the query's execution plan. The value is a comma-delimited string, and can contain any combination of the following values:

  * ``qc_miss``:
    The query was not found in the query cache.

  * ``full_scan``:
    The query performed a full table scan.

  * ``full_join``:
    The query performed a full join (a join without indexes).

  * ``tmp_table``:
    The query created an implicit internal temporary table.

  * ``tmp_table_on_disk``:
    The query``s temporary table was stored on disk.

  * ``filesort``:
    The query used a filesort.

  * ``filesort_on_disk``:
    The filesort was performed on disk.

Values are OR'ed together. If the string is empty, then the filter is disabled. If it is not empty, then queries will only be logged to the slow log if their execution plan matches one of the types of plans present in the filter.

For example, to log only queries that perform a full table scan, set the value to ``full_scan``. To log only queries that use on-disk temporary storage for intermediate results, set the value to ``tmp_table_on_disk,filesort_on_disk``.

.. variable:: log_slow_rate_type

     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: Yes
     :vartype: Enumerated
     :default: ``session``
     :range: ``session``, ``query``

Specifies semantic of :variable:`log_slow_rate_limit` - ``session`` or ``query``.

.. variable:: log_slow_rate_limit

     :cli: Yes
     :conf: Yes
     :scope: Global, session
     :dyn: Yes

Behavior of this variable depends from :variable:`log_slow_rate_type`.

Specifies that only a fraction of ``session/query`` should be logged. Logging is enabled for every nth ``session/query``. By default, n is 1, so logging is enabled for every ``session/query``. Please note: when :variable:`log_slow_rate_type` is ``session`` rate limiting is disabled for the replication thread.

Logging all queries might consume I/O bandwidth and cause the log file to grow large.
 * When :variable:`log_slow_rate_type` is ``session``, this option lets you log full sessions, so you have complete records of sessions for later analysis; but you can rate-limit the number of sessions that are logged. Note that this feature will not work well if your application uses any type of connection pooling or persistent connections. Note that you change :variable:`log_slow_rate_limit` in ``session`` mode, you should reconnect for get effect.

 * When :variable:`log_slow_rate_type` is ``query``, this option lets you log just some queries for later analysis. For example, if you set the value to 100, then one percona of queryies will logged.

Note that every query has global unique ``query_id`` and every connection can has it own (session) :variable:``log_slow_rate_limit``.
Decision "log or no" calculated in following manner:

 * if ``log_slow_rate_limit`` is 0 - log every query

 * If ``log_slow_rate_limit`` > 0 - log query when (``query_id`` % ``log_slow_rate_limit``) is zero.

This allows flexible setup logging behavior.

For example, if you set the value to 100, then one percent of ``sessions/queries`` will be logged.

.. variable:: log_slow_slave_statements

     :cli: Yes
     :conf: Yes
     :scope: Global, session
     :dyn: Yes (in 5.1 releases only)

Specifies that slow queries replayed by the slave SQL thread on a |MySQL| slave will be logged. Upstream version of the |MySQL| server has implemented command line option with same name. Significant difference is that this feature is implemented as variable in |Percona Server|, that means it can be enabled/disabled dynamically without restarting the server.

To start the logging from the slave thread, you should change the global value: set global :variable:`log_slow_slave_statements` ``=ON``; and then execute: ``STOP SLAVE; START SLAVE;``. This will destroy and recreate the slave SQL thread, so it will see the newly set global value.

To stop the logging from the slave thread, you should just change the global value: set global :variable:`log_slow_slave_statements` ``=OFF``; the logging stops immediately.


.. variable:: log_slow_sp_statements

     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: Yes
     :vartype: Boolean
     :default: TRUE
     :range: TRUE/FALSE

If ``TRUE``, statements executed by stored procedures are logged to the slow if it is open.

.. note::

 Support for logging stored procedures doesn't involve triggers, so they won't be logged even if this feature is enabled.

.. variable:: log_slow_verbosity

     :version 5.5.8-20.0: Added ``profiling`` and ``profiling_use_getrusage``
     :cli: Yes
     :conf: Yes
     :scope: Global, session
     :dyn: Yes

Specifies how much information to include in your slow log. The value is a comma-delimited string, and can contain any combination of the following values:

  * ``microtime``:
    Log queries with microsecond precision.

  * ``query_plan``:
    Log information about the query's execution plan.

  * ``innodb``:
    Log |InnoDB| statistics.

  * ``minimal``:
    Equivalent to enabling just ``microtime``.

  * ``standard``:
    Equivalent to enabling ``microtime,innodb``.

  * ``full``:
    Equivalent to all other values OR'ed together.

  * ``profiling``:
    Enables profiling of all queries in all connections.

  * ``profiling_use_getrusage``:
    Enables usage of the getrusage function.

Values are OR'ed together.

For example, to enable microsecond query timing and |InnoDB| statistics, set this option to ``microtime,innodb`` or ``standard``. To turn all options on, set the option to ``full``.

.. variable:: slow_query_log_timestamp_always

     :version 5.5.10-20.1: Introduced  (renamed from :variable:`log_slow_timestamp_every`)
     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: Yes
     :vartype: Boolean
     :default: FALSE
     :range: TRUE/FALSE

If ``TRUE``, a timestamp is printed on every slow log record. Multiple records may have the same time.

.. variable:: slow_query_log_timestamp_precision

     :version 5.5.10-20.1: Introduced (renamed from ``slow_query_log_microseconds_timestamp``)
     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: Yes
     :vartype: Enumerated
     :default: ``second``
     :range: ``second``, ``microsecond``

Normally, entries to the slow query log are in seconds precision, in this format: ::

  # Time: 090402 9:23:36 # User@Host: XXX @ XXX [10.X.X.X]

If :variable:`slow_query_log_timestamp_precision` ``=microsecond``, entries to the slow query log are in microsecond precision, in this format: ::

  # Time: 090402 9:23:36.123456 # User@Host: XXX @ XXX [10.X.X.X]

.. variable:: slow_query_log_use_global_control

     :cli: Yes
     :conf: Yes
     :scope: Global
     :dyn: Yes
     :default: None
     :version 5.5.10-20.1: Introduced (renamed from :variable:`log_slow_timestamp_every`)

Specifies which variables have global scope instead of local. Value is a “flag” variable - you can specify multiple values separated by commas

  * ``none``:
    All variables use local scope

  * ``log_slow_filter``:
    Global variable :variable:`log_slow_filter` has effect (instead of local)

  * ``log_slow_rate_limit``:
    Global variable :variable:`log_slow_rate_limit` has effect (instead of local)

  * ``log_slow_verbosity``:
    Global variable :variable:`log_slow_verbosity` has effect (instead of local)

  * ``long_query_time``:
    Global variable :variable:`long_query_time` has effect (instead of local)

  * ``min_examined_row_limit``:
    Global variable ``min_examined_row_limit`` has effect (instead of local)

  * ``all``
    Global variables has effect (instead of local)

**NOTE:** This variable has been renamed from :variable:`log_slow_timestamp_every` since 5.5.10-20.1.


Other Information
=================

Changes to the Log Format
-------------------------

The feature adds more information to the slow log output. Here is a sample log entry: ::

  # User@Host: mailboxer[mailboxer] @  [192.168.10.165]
  # Thread_id: 11167745  Schema: board
  # QC_Hit: No  Full_scan: No  Full_join: No  Tmp_table: Yes  Disk_tmp_table: No
  # Filesort: Yes  Disk_filesort: No  Merge_passes: 0
  # Query_time: 0.000659  Lock_time: 0.000070  Rows_sent: 0  Rows_examined: 30  Rows_affected: 0  Rows_read: 30
  #   innodb_IO_r_ops: 1  innodb_IO_r_bytes: 16384  innodb_IO_r_wait: 0.028487
  #   innodb_rec_lock_wait: 0.000000  innodb_queue_wait: 0.000000
  #   innodb_pages_distinct: 5
  select count(distinct author_id) from art87.article87 force index (forum_id) where forum_id = 240215 and thread_id = ``710575`` 

Another example (:variable:`log_slow_verbosity` ``=profiling``): ::

  # Query_time: 4.555235  Lock_time: 0.000000  Rows_sent: 1  Rows_examined: 0  Rows_affected: 0  Rows_read: 1
  # Profile_starting: 4.554799 Profile_starting_cpu: 0.000000 Profile_checking_permissions: 0.000095 Profile_checking_permissions_cpu: 0.000000 Profile_Opening_tables: 0.000088 Profile_Opening_tables_cpu: 0.000000 Profile_init: 0.000056 Profile_init_cpu: 0.000000 Profile_optimizing: 0.000046 Profile_optimizing_cpu: 0.000000 Profile_executing: 0.000098 Profile_executing_cpu: 0.000000 Profile_end: 0.000049 Profile_end_cpu: 0.000000 Profile_query_end: 0.000045 Profile_query_end_cpu: 0.000000 Profile_freeing_items: 0.000084 Profile_freeing_items_cpu: 0.000000 Profile_logging_slow_query: 0.000045 Profile_logging_slow_query_cpu: 0.000000 
  # Profile_total: 4.555405 Profile_total_cpu: 0.000000 
  insert into teee4 select * from teee4 limit 10000000;

Connection and Schema Identifier
--------------------------------

Each slow log entry now contains a connection identifier, so you can trace all the queries coming from a single connection. This is the same value that is shown in the Id column in ``SHOW FULL PROCESSLIST`` or returned from the ``CONNECTION_ID()`` function.

Each entry also contains a schema name, so you can trace all the queries whose default database was set to a particular schema. ::

  # Thread_id: 11167745  Schema: board

Microsecond Time Resolution and Extra Row Information
-----------------------------------------------------

This is the original functionality offered by the ``microslow`` feature. ``Query_time`` and ``Lock_time`` are logged with microsecond resolution.

The feature also adds information about how many rows were examined for ``SELECT`` queries, and how many were analyzed and affected for ``UPDATE``, ``DELETE``, and ``INSERT`` queries, ::

  # Query_time: 0.000659  Lock_time: 0.000070  Rows_sent: 0  Rows_examined: 30  Rows_affected: 0  Rows_read: 30

Values and context:

  * ``Rows_examined``:
    Number of rows scanned - ``SELECT``

  * ``Rows_affected``:
    Number of rows changed - ``UPDATE``, ``DELETE``, ``INSERT``

  * ``Rows_read``:
    Number of rows read - ``UPDATE``, ``DELETE``, ``INSERT``

Memory Footprint
----------------

The feature provides information about the amount of bytes sent for the result of the query and the number of temporary tables created for its execution - differentiated by whether they were created on memory or on disk - with the total number of bytes used by them. ::

  # Bytes_sent: 8053  Tmp_tables: 1  Tmp_disk_tables: 0  Tmp_table_sizes: 950528

Values and context:

  * ``Bytes_sent``:
    The amount of bytes sent for the result of the query

  * ``Tmp_tables``:
    Number of temporary tables created on memory for the query

  * ``Tmp_disk_tables``:
    Number of temporary tables created on disk for the query

  * ``Tmp_table_sizes``:
    Total Size in bytes for all temporary tables used in the query


Query Plan Information
----------------------

Each query can be executed in various ways. For example, it may use indexes or do a full table scan, or a temporary table may be needed. These are the things that you can usually see by running ``EXPLAIN`` on the query. The feature will now allow you to see the most important facts about the execution in the log file. ::

  # QC_Hit: No  Full_scan: No  Full_join: No  Tmp_table: Yes  Disk_tmp_table: No
  # Filesort: Yes  Disk_filesort: No  Merge_passes: 0

The values and their meanings are documented with the :variable:`log_slow_filter` option.

|InnoDB| Usage Information
--------------------------

The final part of the output is the |InnoDB| usage statistics. |MySQL| currently shows many per-session statistics for operations with ``SHOW SESSION STATUS``, but that does not include those of |InnoDB|, which are always global and shared by all threads. This feature lets you see those values for a given query. ::

  # innodb_IO_r_ops: 1  innodb_IO_r_bytes: 16384  innodb_IO_r_wait: 0.028487
  # innodb_rec_lock_wait: 0.000000  innodb_queue_wait: 0.000000
  # innodb_pages_distinct: 5

Values:

  * ``innodb_IO_r_ops``:
    Counts the number of page read operations scheduled. The actual number of read operations may be different, but since this can be done asynchronously, there is no good way to measure it.

  * ``innodb_IO_r_bytes``:
    Similar to innodb_IO_r_ops, but the unit is bytes.

  * ``innodb_IO_r_wait``:
    Shows how long (in seconds) it took |InnoDB| to actually read the data from storage.

  * ``innodb_rec_lock_wait``:
    Shows how long (in seconds) the query waited for row locks.

  * ``innodb_queue_wait``:
    Shows how long (in seconds) the query spent either waiting to enter the |InnoDB| queue or inside that queue waiting for execution.

  * ``innodb_pages_distinct``:
    Counts approximately the number of unique pages the query accessed. The approximation is based on a small hash array representing the entire buffer pool, because it could take a lot of memory to map all the pages. The inaccuracy grows with the number of pages accessed by a query, because there is a higher probability of hash collisions.

If the query did not use |InnoDB| tables, that information is written into the log instead of the above statistics.

Related Reading
===============

  * http://www.mysqlperformanceblog.com/2009/02/10/impact-of-logging-on-mysql%E2%80%99s-performance/

  * `log_slow_filter Usage <http://www.mysqlperformanceblog.com/2008/09/22/finding-what-created_tmp_disk_tables-with-log_slow_filter/>`_

  * `Blueprint in Launchpad <https://blueprints.launchpad.net/percona-server/+spec/microseconds-in-query-log>`_
