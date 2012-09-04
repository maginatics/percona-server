=============================
Changed in Percona Server 5.6
=============================

|Percona Server| 5.6 is based on MySQL 5.6 and incorporates many of the improvements found in |Percona Server| 5.5.

Some features that were present in |Percona Server 5.5| have been removed in |Percona Server 5.6|. These are:

 * optimizer_fix
 * fast_index_creation (use MySQL 5.6's ALGORITHM= option instead)
 * HandlerSocket (may return when HandlerSocket supports MySQL 5.6)
 * SHOW [GLOBAL] TEMPORARY TABLES functionality is now only available via the INFORMATION_SCHEMA tables TEMPORARY_TABLES and GLOBAL_TEMPORARY_TABLES.

Some features that were present in |Percona Server 5.5| have been replaced by a different implementation of the same/similar functionality in |Percona Server 5.6|. These are:

 * SHOW INNODB STATUS section "OLDEST VIEW" has been replaced by the XTRADB_READ_VIEW INFORMATION_SCHEMA table.

