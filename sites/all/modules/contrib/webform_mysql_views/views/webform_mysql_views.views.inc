<?php
/**
 * @file
 */

/**
 * Implements hook_views_data_alter().
 *
 * @param $data
 *   An array of views data structures.
 */
function webform_mysql_views_views_data_alter(&$data) {
  if (module_exists('data')) {
    // expose the webform_mysql_views by designating a "pseudo"-primary key
    $webform_mysql_views = variable_get('webform_mysql_views_views', array());
    foreach ($webform_mysql_views as $nid => $dbview_name) {
      $node = node_load($nid);

      // sid (submission id) is unique
      _webform_mysql_views_designate_primarykey($dbview_name, 'sid', $data);

      // Prettify the field group name in the Views UI.
      $data[$dbview_name]['table']['group'] = $data[$dbview_name]['base']['title'] = $data[$dbview_name]['table']['base']['title'] = t('Webform: @title', array('@title' => $node->title));

      // Add an implicit relationship to the main webform submissions table
      $data[$dbview]['table']['join'] = array(
        'webform_submissions' => array(
          'left_field' => 'sid',
          'field' => sid,
        ),
      );

      // Prettify the sid field in the UI.
      $data[$dbview_name]['sid']['title'] = t('Submission ID');
      $data[$dbview_name]['sid']['help'] = t('The unique ID of the submission.');
      $data[$dbview_name]['sid']['relationship'] = array(
        'base' => 'webform_submissions',
        'field' => 'sid',
        'handler' => 'views_handler_relationship',
        'label' => t('Webform submissions'),
      );

      // Set the correct label on the fields.
      $result = db_query('SELECT c.cid, c.name, c.form_key, p.form_key AS parent_key, p.name AS parent, c.type FROM {webform_component} c LEFT JOIN {webform_component} p ON (c.pid = p.cid) WHERE c.nid = %d AND c.type != "fieldset" ORDER BY c.weight ASC, c.cid ASC', $nid);
      while ($row = db_fetch_array($result)) {
        $column = _webform_mysql_views_column_name($row);

        $data[$dbview_name][$column]['title'] = empty($row['parent']) ? $row['name'] : $row['name'] .' ('. $row['parent'] .')';
        if (empty($row['parent'])) {
          $data[$dbview_name][$column]['field']['help'] = t('The @field field on the @webform webform.', array('@field' => $row['name'], '@webform' => $node->title));
        }
        else {
          $data[$dbview_name][$column]['field']['help'] = t('The @field field in the @parent fieldset on the @webform webform.', array('@field' => $row['name'], '@webform' => $node->title, '@parent' => $row['parent']));
        }

        // Add handlers for numeric fields.
        if ($row['type'] == 'number') {
          $data[$dbview_name][$column]['field']['handler'] = 'views_handler_field_numeric';
          $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
          $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort_numeric';
          $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_group_by_numeric';
          $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_argument_numeric';
        }
        else if ($row['type'] == 'select') {
          // Select field - show the selectable options.
          $data[$dbview_name][$column]['field'] = array(
            'handler' => 'webform_mysql_views_handler_field_select',
            'options callback' => '_webform_mysql_views_select_options',
            'options arguments' => array($nid, $row['cid']),
          );
          $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
          $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort';
          $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_argument_string';

          // The views 2.x in_operator filter has no options arguments, so
          // we have to use our own if we want to provide arguments to the
          // callback function.
          $data[$dbview_name][$column]['filter'] = array(
            'handler' => (views_api_version() == 3) ? 'views_handler_filter_in_operator' : 'webform_mysql_views_handler_select_filter',
            'options callback' => '_webform_mysql_views_select_options',
            'options arguments' => array($nid, $row['cid']),
          );
        }
        else if ($row['type'] == 'file') {
          if (module_exists('upload')) {
            $data[$dbview_name][$column]['field']['handler'] = 'webform_mysql_views_handler_field_file_fid';
            $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort_numeric';
            $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_group_by_numeric';

            $data[$dbview_name][$column]['filter'] = array(
              'handler' => 'webform_mysql_views_handler_filter_file_fid',
              'title' => t('Has attached files'),
              'type' => 'yes-no',
              'help' => t('Only display items with attached files. This can cause duplicates if there are multiple attached files.'),
            );

            // Add an implicit relationship to the files table.
            $data[$dbview_name][$column]['relationship'] = array(
              'title' => t('File Component @field', array('@field' => $row['name'])),
              'help' => t('Add a relationship to gain access to more file data for files uploaded through the File component. Note that this relationship will cause duplicate results if there are multiple files attached to the webform submission.'),
              'base' => 'files',
              'base field' => 'fid',
              'handler' => 'views_handler_relationship',
              'label' => t('@title: File Component', array('@title' => $node->title)),
            );
          }
          else {
            // if upload.module is not present.
            $data[$dbview_name][$column]['field']['handler'] = 'views_handler_field_numeric';
            $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
            $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort_numeric';
            $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_group_by_numeric';
            $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_argument_numeric';
          }
        }
        else if (module_exists('date_api') && $row['type'] == 'date') {
          // Make date fields a bit more useful.
          $data[$dbview_name][$column]['field']['handler'] = 'views_handler_field_date';
          $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
          $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort_date';
          $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_date';
          $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_sort_date';
        }
        else if ($row['type'] == 'node') {
          // If the user happens to have webform_node_element installed.
          $data[$dbview_name][$column]['field']['handler'] = 'webform_mysql_views_handler_field_node';
          $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
          $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort';
          $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_numeric';
          $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_argument_node_nid';

          // Add an implicit relationship to the node table.
          $data[$dbview_name][$column]['relationship'] = array(
            'title' => t('Node Component @field', array('@field' => $row['name'])),
            'help' => t('Add a relationship to gain access to more file data for files uploaded through the File component. Note that this relationship will cause duplicate results if there are multiple files attached to the webform submission.'),
            'base' => 'node',
            'base field' => 'nid',
            'handler' => 'views_handler_relationship',
            'label' => t('@title: Node Component', array('@title' => $node->title)),
          );
        }
        else {
          // Others fields are explicitly set to text.
          $data[$dbview_name][$column]['field']['handler'] = 'views_handler_field';
          $data[$dbview_name][$column]['field']['click sortable'] = TRUE;
          $data[$dbview_name][$column]['sort']['handler'] = 'views_handler_sort';
          $data[$dbview_name][$column]['filter']['handler'] = 'views_handler_filter_string';
          $data[$dbview_name][$column]['argument']['handler'] = 'views_handler_argument_string';
        }
      }

      // Added to convert the submitted field into a date field handler.
      $data[$dbview_name]['submitted'] = array(
        'title' => t('submitted'),
        'help' => t('The date and time the webform was submitted.'),
        'field' => array(
          'handler' => 'views_handler_field_date',
          'click sortable' => TRUE,
        ),
        'sort' => array(
          'handler' => 'views_handler_sort_date',
        ),
        'filter' => array(
          'handler' => 'views_handler_filter_date',
        ),
        'argument' => array(
          'handler' => 'views_handler_argument_date',
        ),
      );
    }
  }
}

/**
 * Implements hook_views_handlers()
 */
function webform_mysql_views_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'webform_mysql_views') . '/views/handlers',
    ),
    'handlers' => array(
      'webform_mysql_views_handler_field_select' => array(
        'parent' => 'views_handler_field',
      ),
      'webform_mysql_views_handler_select_filter' => array(
        'parent' => 'views_handler_filter_in_operator',
      ),
      'webform_mysql_views_handler_field_file_fid' => array(
        'parent' => 'views_handler_field_prerender_list',
      ),
      'webform_mysql_views_handler_filter_file_fid' => array(
        'parent' => 'views_handler_filter_boolean_operator',
      ),
      'webform_mysql_views_handler_field_date' => array(
        'parent' => 'views_handler_field_date',
      ),
      'webform_mysql_views_handler_field_node' => array(
        'parent' => 'views_handler_field',
      ),
    ),
  );
}

/**
 * Implements hook_views_plugins().
function webform_mysql_views_views_plugins() {
  $plugins = array(
    'module' => 'views', // This just tells our themes are elsewhere.
    'row' => array(
      'webform_mysql_views_rss' => array(
      'title' => t('Fields'),
      'help' => t('Display the node with standard node view.'),
      'handler' => 'views_plugin_row_fields',
       // 'path' => drupal_get_path('module', 'views') . '/modules/node', // not necessary for most modules
      'theme' => 'views_view_row',
      'base' => variable_get('webform_mysql_views_views', array()),
      'uses options' => TRUE,
      'type' => 'feed',
      'help topic' => 'style-webform-mysql-views-rss',
      ),
    ),
  );

  // return $plugins;
}
*/

/**
 * Implements hook_views_plugins_alter().
function webform_mysql_views_views_plugins_alter(&$plugins) {
  dpm($plugins);
}
*/
