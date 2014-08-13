<?php
/**
 * @file webform_mysql_views_Handler_select_filter.inc
 *
 * Simple filter to handle matching of multiple options selectable via checkboxes
 *
 * Definition items:
 * - numeric: If set to true, this item will use numeric operators instead of string.
 * - options callback: The function to call in order to generate the value options. If omitted, the options 'Yes' and 'No' will be used.
 * - options arguments: An array of arguments to pass to the options callback.
 *
 */
class webform_mysql_views_handler_select_filter extends views_handler_filter_in_operator {

  /**
   * Views 6.x-2.x doesn't include the 'options arguments' so we need to
   * include our own handler to provide it. Bum.
   */
  function get_value_options() {
    if (isset($this->value_options)) {
      return;
    }

    if (isset($this->definition['options callback']) && is_callable($this->definition['options callback'])) {
      if (isset($this->definition['options arguments']) && is_array($this->definition['options arguments'])) {
        $this->value_options = call_user_func_array($this->definition['options callback'], $this->definition['options arguments']);
      }
      else {
        $this->value_options = call_user_func($this->definition['options callback']);
      }
    }
    else {
      $this->value_options = array(t('Yes'), t('No'));
    }
  }
}
