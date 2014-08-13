<?php
/**
 * Field handler to provide a list of files.
 */
class webform_mysql_views_handler_field_file_fid extends views_handler_field_prerender_list {
  function construct() {
    parent::construct();
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['link_to_file'] = array('default' => FALSE);
    $options['only_listed'] = array('default' => FALSE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['link_to_file'] = array(
      '#title' => t('Link this field to download the file'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_file']),
    );
  }

  function pre_render($values) {
    $fids = array();
    $this->items = array();

    foreach ($values as $result) {
      if (!empty($result->{$this->field_alias})) {
        $fids[] = $result->{$this->field_alias};
      }
       else {
         $fids[] = 0;
       }
    }

    if ($fids) {
      $result = db_query("SELECT f.fid, f.filename, f.filepath, f.filesize, f.filemime FROM {files} f WHERE f.fid IN (". implode(', ', $fids) .")");
      while ($file = db_fetch_array($result)) {
        $file['filename'] = check_plain($file['filename']);
        $file['filemime'] = check_plain($file['filemime']);
        $file['filesize'] = format_size($file['filesize']);
        $file['filepath'] = file_create_url($file['filepath']);
        if (!empty($this->options['link_to_file']) ) {
          $file['make_link'] = TRUE;
          $file['path'] = $file['filepath'];
        }
        $this->items[$file['fid']][$file['fid']] = $file;
      }
    }
  }

  function render_item($count, $item) {
    return $item['filepath'];
  }

  function document_self_tokens(&$tokens) {
    $tokens['[' . $this->options['id'] . '-fid' . ']'] = t('The file ID for the file.');
    $tokens['[' . $this->options['id'] . '-name' . ']'] = t('The name of the attached file.');
    $tokens['[' . $this->options['id'] . '-type' . ']'] = t('The MIME type of the attached file.');
    $tokens['[' . $this->options['id'] . '-path' . ']'] = t('The path of the attached file.');
    $tokens['[' . $this->options['id'] . '-url' . ']'] = t('The url of the attached file.');
    $tokens['[' . $this->options['id'] . '-size' . ']'] = t('The size of the attached file.');
  }

  function add_self_tokens(&$tokens, $item) {
    $tokens['[' . $this->options['id'] . '-fid' . ']'] = $item['fid'];
    $tokens['[' . $this->options['id'] . '-name' . ']'] = $item['filename'];
    $tokens['[' . $this->options['id'] . '-type' . ']'] = $item['filemime'];
    $tokens['[' . $this->options['id'] . '-path' . ']'] = $item['filepath'];
    $tokens['[' . $this->options['id'] . '-url' . ']'] = url($item['filepath']);
    $tokens['[' . $this->options['id'] . '-size' . ']'] = $item['filesize'];
  }
}
