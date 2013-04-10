.. _innodb_fast_index_creation:

=====================
 Fast Index Creation
=====================

Percona has implemented several changes related to |MySQL|'s fast index creation feature. Extended features, besides disabling :variable:`fast_index_creation`, can be enabled with :variable:`expand_fast_index_creation`.

Disabling Fast Index Creation
=============================

Fast index creation was implemented in |MySQL| as a way to speed up the process of adding or dropping indexes on tables with many rows. However, cases have been found in which fast index creation creates an inconsistency between |MySQL| and |InnoDB| data dictionaries.

This feature implements a session variable that disables fast index creation. This causes indexes to be created in the way they were created before fast index creation was implemented. While this is slower, it avoids the problem of data dictionary inconsistency between |MySQL| and |InnoDB|.


``OPTIMIZE TABLE``
------------------

Internally, ``OPTIMIZE TABLE`` is mapped to ``ALTER TABLE ... ENGINE=innodb`` for |InnoDB| tables. As a consequence, it now also benefits from fast index creation, with the same restrictions as for ``ALTER TABLE``.


Version Specific Information
============================

  * :rn:`5.1.49-rel12.0`: 
    Variable :variable:`fast_index_creation` implemented.

  * :rn:`5.1.56-12.7`:
    Expanded the applicability of fast index creation to :command:`mysqldump`, ``ALTER TABLE``, and ``OPTIMIZE TABLE``.


System Variables
================

.. variable:: fast_index_creation

     :cli: Yes
     :conf: No
     :scope: Local
     :dyn: Yes
     :vartype: Boolean
     :default: ON
     :range: ON/OFF


Other Reading
=============

  * `Thinking about running OPTIMIZE on your InnoDB Table? Stop! <http://www.mysqlperformanceblog.com/2010/12/09/thinking-about-running-optimize-on-your-innodb-table-stop/>`_
