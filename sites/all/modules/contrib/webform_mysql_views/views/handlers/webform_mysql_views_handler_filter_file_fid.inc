<?php

/**
 * Filter by whether or not a webform submission has attached files
 */
class webform_mysql_views_handler_filter_file_fid extends views_handler_filter_boolean_operator {
  function construct() {
    parent::construct();
    $this->value_value = t('Has attached files');
  }

  function query() {
    $this->ensure_my_table();
    $this->query->add_where($this->options['group'], "(SELECT COUNT(f.fid) FROM {files} f WHERE f.fid = $this->table_alias.$this->field) " . (empty($this->value) ? '=' : '<>') . " 0");
  }
}

