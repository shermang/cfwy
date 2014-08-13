<?php
/**
 * @file
 * Contains the webform mysql views 'select element' field handler.
 */

/**
 * Field handler to provide simple renderer that show select values instead of keys.
 */
class webform_mysql_views_handler_field_select extends views_handler_field {
  /**
   * Constructor to provide additional field to add.
   */
  function construct() {
    parent::construct();
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['key'] = array('default' => isset($this->definition['key default']) ? $this->definition['key default'] : FALSE);
    return $options;
  }

  /**
   * Provide link to node option
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['key'] = array(
      '#title' => t('Show key'),
      '#description' => t('Show the raw item key instead of the "translated" item value.'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['key']),
    );
  }

  /**
   * Render whatever the data is according to field options.
   *
   * Data will be made XSS safe in this function.
   */
  function render_data($data) {
    if (isset($this->definition['options callback']) && is_callable($this->definition['options callback'])) {
      if (isset($this->definition['options arguments']) && is_array($this->definition['options arguments'])) {
        $this->values = call_user_func_array($this->definition['options callback'], $this->definition['options arguments']);
      }
      else {
        $this->values = call_user_func($this->definition['options callback']);
      }
    }

    $selected = array();
    $items = split(',', $data);
    foreach ($items as $item) {
      if (empty($this->options['key']) && !empty($this->values)) {
        $selected[] = check_plain($this->values[$item]);
      }
      else {
        $selected[] = check_plain($item);
      }
    }
    return implode(',', $selected);
  }

  function render($values) {
    return $this->render_data($values->{$this->field_alias});
  }
}
