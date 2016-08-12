<?php

/**
 * Displays the active rules
 */
function commerce_qbo_ui_active_rules() {
  commerce_qbo_ui_ready_check();

  RulesPluginUI::$basePath = 'admin/commerce/commerce_qbo_ui/rules';
  $options = array('show plugin' => FALSE);

  $content['enabled']['title']['#markup'] = '<h3>' . t('Enabled QuickBooks Report rules') . '</h3>';

  $conditions = array('tags' => array('commerce qbo'), 'plugin' => 'reaction rule', 'active' => TRUE);
  $content['enabled']['rules'] = RulesPluginUI::overviewTable($conditions, $options);
  $content['enabled']['rules']['#empty'] = t('There are no active QuickBooks report rules.');

  $content['disabled']['title']['#markup'] = '<h3>' . t('Disabled QuickBooks Report rules') . '</h3>';

  $conditions['active'] = FALSE;
  $content['disabled']['rules'] = RulesPluginUI::overviewTable($conditions, $options);
  $content['disabled']['rules']['#empty'] = t('There are no disabled QuickBooks Report rules.');

  // Store the function name in the content array to make it easy to alter the
  // contents of this page.
  $content['#page_callback'] = 'commerce_qbo_ui_active_rules';

  return $content;
}

/**
 * The form for the Settings tab
 *
 * @TODO: Allow users to decide whether or not this module can create QuickBooks
 *   entities (customers, items).  Users may be more inclined to create these
 *   properties themselves in case of a transaction failure so that have total
 *   control over the set up.
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_config_form($form, &$form_state) {
  commerce_qbo_ui_ready_check();

  // Allow the user to populate certain field data with select lists pull from
  // their quickbooks account.  This is irreversible.
  if (variable_get('commerce_qbo_ui_run', 0)) {
    $form['field_actions'] = array(
      '#type' => 'fieldset',
      '#title' => t('Request QuickBooks data for form fields'),
      '#description' => t('Populates fields in the qb_reports forms with data pulled from QuickBooks, allowing admins to select from a list of acceptable values instead of manually inputting IDs.<br/>Affected fields: <i>Deposit Account, Refund Account, Tax Rate</i><br/><b>WARNING: This process is irreversible.  You will be unable to go back to direct ID input, but you can update the lists by pressing this button again.</b>'),
    );
    $form['field_actions']['commerce_qbo_ui_populate_field_data'] = array(
      '#type' => 'submit',
      '#title' => t('Pull data from QuickBooks'),
      '#value' => t('Populate'),
      '#limit_validation_errors' => array(),
      '#submit' => array('commerce_qbo_ui_populate_field_data'),
    );
  }

  // The section that allows the user to map their drupal taxes to QuickBooks.
  $qb_tax_field = field_info_field('commerce_qbo_ui_tax_type_s');
  $can_use = !empty($qb_tax_field['settings']['allowed_values']);
  $tax_cfg_msg = $can_use
    ? t("Set up the connection between your Drupal tax rates and your QuickBooks tax rates. <br/>Select a Drupal tax from the dropdown(s) to map it to the Quickbooks tax labelled above.")
    : t("You must load your QuickBooks tax types using the above button before you can map your Drupal tax rates to them.");

  $form['commerce_qbo_ui_tax_config'] = array(
    '#type' => 'fieldset',
    '#title' => t('Drupal tax to QuickBooks tax config'),
    '#description' => $tax_cfg_msg,
  );
  if ($can_use) {
    // Grab the list of Drupal tax rates, and convert them to an options array.
    $drupal_tax_rates = commerce_tax_rates();
    $drupal_tax_rate_options = array();
    foreach($drupal_tax_rates as $name => $tax) {
      $drupal_tax_rate_options[$name] = $tax['display_title'];
    }

    // Convenience variable for less code.
    $tax_cfg_index = 'commerce_qbo_ui_tax_config';

    $defaults = variable_get($tax_cfg_index, array());

    foreach ($qb_tax_field['settings']['allowed_values'] as $tax_code => $name) {
      $form[$tax_cfg_index]['tax_code_' . $tax_code] = array(
        '#type' => 'select',
        '#title' => $name,
        '#options' => $drupal_tax_rate_options,
        '#default_value' => empty($defaults[$tax_code]) ?: $defaults[$tax_code],
        '#prefix' => '<span style="display: inline-block; padding: 0 10px;">',
        '#suffix' => '</span>',
      );
    }
  }

  // For capturing fail/success messages from QuickBooks Online.
  $form['commerce_qbo_ui_logging'] = array(
    '#description' => t('Enable/disable message logging from QuickBooks API calls.'),
    '#type' => 'radios',
    '#title' => t('Save data-transfer logs'),
    '#options' => array(
      0 => t("No"),
      1 => t("Yes"),
    ),
    '#default_value' => variable_get('commerce_qbo_ui_logging', 0),
    '#required' => TRUE,
  );

  // A safety net for transferring reports to staging.
  $form['commerce_qbo_ui_stage_premature_reports'] = array(
    '#description' => t('If enabled, any reports that are sent before the module is fully configured will be caught and passed into staging instead.<br><em>You will still be unable to send these reports until configuration is complete.</em>'),
    '#type' => 'radios',
    '#title' => t('Catch reports sent too early'),
    '#options' => array(
      0 => t('No'),
      1 => t('Yes'),
    ),
    '#default_value' => variable_get('commerce_qbo_ui_stage_premature_reports', 1),
    '#required' => TRUE,
  );

  // Warn privileged users when reports are pending action.
  $form['commerce_qbo_ui_alert_pending_reports'] = array(
    '#description' => t('Optionally display a persistent notification when reports are in the staging area.'),
    '#type' => 'radios',
    '#title' => t('Display notification when reports are in the staging queue'),
    '#options' => array(
      0 => t('No'),
      1 => t('Yes'),
    ),
    '#default_value' => variable_get('commerce_qbo_ui_alert_pending_reports', 0),
    '#required' => TRUE,
  );

  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));

  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }
  $form['#submit'][] = 'commerce_qbo_ui_config_form_submit';
  // By default, render the form using theme_system_settings_form().
  if (!isset($form['#theme'])) {
    $form['#theme'] = 'system_settings_form';
  }

  return $form;
}

/**
 * Form submission handler for commerce_qbo_ui_config_form().
 *
 * Custom submit handler created to help store QB tax mappings.
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_config_form_submit($form, &$form_state) {
  // Exclude unnecessary elements.
  form_state_values_clean($form_state);

  // Group all tax mappings together.
  $tax_codes = array();

  foreach ($form_state['values'] as $key => $value) {
    if (strpos($key, 'tax_code_') === 0) {
      $code = substr($key, 9);

      // We set both values here so we can set the default/saved values for the
      // form based on QB tax code, and query the QB code later with the Drupal
      // tax attached to the order.
      $tax_codes[$code] = $value;
      $tax_codes[$value] = $code;
      continue;
    }
    else if (is_array($value) && isset($form_state['values']['array_filter'])) {
      $value = array_keys(array_filter($value));
    }
    variable_set($key, $value);
  }

  if (!empty($tax_codes)) {
    variable_set('commerce_qbo_ui_tax_config', $tax_codes);
  }

  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Submit handler for commerce_qbo_ui_populate_field_data button
 *
 * Creates or updates the
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_populate_field_data($form, &$form_state) {
  // We should only be able to access this function if the api is fully
  // configured, but double check anyways.
  if (qbo_api_client()) {
    // Grab QuickBooks values.
    $accounts = qbo_api_query_as_options('SELECT Id, Name FROM Account', '', TRUE);
    $tax_codes = qbo_api_query_as_options('SELECT Id, Name FROM TaxCode', '', TRUE);
    $terms = qbo_api_query_as_options("SELECT Id, Name FROM Term", '- Select One -', TRUE);

    // Edit field values.
    $pre = "commerce_qbo_ui";
    $field = array();

    $field['field_name'] = $pre . "_deposit_acc_s";
    $field['settings']['allowed_values'] = $accounts;
    field_update_field($field);

    $field['field_name'] = $pre . "_refund_acc_s";
    field_update_field($field);

    $field['field_name'] = $pre . "_tax_type_s";
    $field['settings']['allowed_values'] = $tax_codes;
    field_update_field($field);

    $field['field_name'] = $pre . "_sales_term_s";
    $field['settings']['allowed_values'] = $terms;
    field_update_field($field);

    drupal_set_message(t("Fields populated successfully."));
  }
  else {
    drupal_set_message(t("Failed to populate lists."));
  }
}

/**
 * Displays a list of reports waiting to be manually submitted to QuickBooks
 *
 * A table with the following features is printed out:
 *  - View order (the order number links back to the view order screen)
 *  - Select report type to send (a dropdown containing any user created report types)
 *  - Remove order (removes from the pending list, a cron will clear the database)
 *  - Send report (Sends the report in the chosen format.  Throws validation errors if no format is selected)
 *
 * If the QBO API is missing the necessary app tokens then the module will not allow reports to be sent
 */
function commerce_qbo_ui_pending_reports($form, $form_state) {
  commerce_qbo_ui_ready_check();

  // Remove and deleted rows.
  $trigger = isset($form_state['triggering_element']) ? $form_state['triggering_element'] : NULL;

  if ($trigger != NULL) {
    if ($trigger['#value'] == 'Remove') {
      $srid = str_replace('commerce_qbo_ui_remove_', '',$trigger['#name']);

      // Convert the ID to an int.
      $srid *= 1;

      _commerce_qbo_ui_clear_sr_by_id($srid);
    }
    elseif ($trigger['#value'] == 'Submit') {
      $srid = str_replace('commerce_qbo_ui_submit_', '', $trigger['#name']);

      // Convert the ID to an int.
      $srid *= 1;

      $result = db_select('commerce_qbo_ui_staged_report', 'sr')
        ->fields('sr', array('order_id'))
        ->condition('rid', $srid, '=')
        ->execute()
        ->fetchAssoc();

      $order_id = $result['order_id'];
      $report_type = $form_state['values']['commerce_qbo_ui_report_type'];
      $notice = '';

      if (commerce_qbo_ui_send_report($order_id, $report_type)) {
        // We can remove the report from pending if the transaction is successful.
        _commerce_qbo_ui_clear_sr_by_id($srid);

        if (user_access('view commerce qbo ui messages')) {
          $alert = drupal_get_messages('commerce_qbo_ui');
          foreach ($alert['commerce_qbo_ui'] as $msg) {
            $notice .= '<div class="messages status">' . $msg . '</div>';
          }
        }
      }
      else {
        if (user_access('view commerce qbo ui messages')) {
          $alert = drupal_get_messages('commerce_qbo_ui');
          foreach ($alert['commerce_qbo_ui'] as $msg) {
            $notice .= '<div class="messages error">' . $msg . '</div>';
          }
        }
      }

      $build['commerce_qbo_ui_alert'] = array(
        '#markup' => $notice,
      );
    }
  }

  $reports_pending = db_select('commerce_qbo_ui_staged_report', 'sr')
    ->fields('sr', array('rid', 'order_id', 'memo'))
    ->condition('deleted', 0, '=')
    ->execute();

  $report_types = db_select('commerce_qbo_ui_report', 'rt')
      ->fields('rt', array('rtid', 'name'))
      ->condition('status', 1, '=')
      ->execute();

  $rows = _commerce_qbo_ui_generate_sr_rows($reports_pending, $report_types);

  $build['commerce_qbo_ui_table'] = array(
    '#theme' => 'commerce_qbo_ui_staged_reports_table',
    'rows' => $rows,
  );
  $build['commerce_qbo_ui_pager'] = array('#theme' => 'pager');

  return $build;
}

/**
 * Displays all existing QuickBooks Reports, allowing users to edit,
 * delete or add new Reports
 */
function commerce_qbo_ui_reports_overview() {
  // Get all created qb_reports.
  $query = db_select('commerce_qbo_ui_report', 'r');
  $query->leftJoin('commerce_qbo_ui_report_type', 'rt', 'r.report_type = rt.type');
  $results = $query
    ->fields('r', array('rtid', 'name'))
    ->fields('rt', array('name'))
    ->condition('r.status', 1, '=')
    ->execute();

  $header = array(
    t('Name'),
    t('Type of Report'),
    array('data' => t('Operations'), 'colspan' => '2'),
  );
  $rows = array();

  while($qb_report = $results->fetchAssoc()) {
    $name = $qb_report['name'];

    $row = array($name);
    $row[] = $qb_report['rt_name'];

    // Set the op columns.
    $row[] = array('data' => l(t('edit'), 'admin/commerce/commerce_qbo_ui/qb_reports/' . $qb_report['rtid']));
    $row[] = array('data' => l(t('delete'), 'admin/commerce/commerce_qbo_ui/qb_reports/' . $qb_report['rtid'] . '/delete'));


    $rows[] = $row;
  }

  $build['commerce_qbo_ui_qb_report_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No reports available. !link', array('!link' => l(t('Add a report.'), 'admin/commerce/commerce_qbo_ui/qb_reports/add'))),
  );

  return $build;
}

/**
 * Prints any logs returned from QuickNooks
 */
function commerce_qbo_ui_display_logs() {
  commerce_qbo_ui_ready_check();

  $rows = array();
  $header = array(
    '', // Icon column.
    array('data' => t('Date'), 'field' => 'l.mid', 'sort' => 'desc'),
    t('Message'),
  );

  $query = db_select('commerce_qbo_ui_message_log', 'l')->extend('PagerDefault')->extend('TableSort');
  $query->fields('l', array('mid', 'message', 'timestamp',));

  $result = $query
    ->limit(50)
    ->orderByHeader($header)
    ->execute();

  foreach ($result as $qbo_log) {
    $rows[] = array('data' =>
      array(
        // Cells.
        array('class' => 'icon'),
        format_date($qbo_log->timestamp, 'short'),
        $qbo_log->message,
      ),
      // Attributes for tr.
      'class' => array(drupal_html_class('qbo-log-message')),
    );
  }

  $build['commerce_qbo_ui_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array('id' => 'admin-commerce-qbo-ui-log'),
    '#empty' => t('No log messages available.'),
  );
  $build['commerce_qbo_ui_pager'] = array('#theme' => 'pager');

  return $build;
}
