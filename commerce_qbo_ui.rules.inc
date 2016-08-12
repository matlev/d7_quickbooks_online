<?php

/**
 * Implements hook_rules_event_info()
 *
 * Defines events triggered by the commerce qbo ui module
 *
 * @return array
 */
function commerce_qbo_ui_rules_event_info() {
  return array(
    'commerce_qbo_ui_report_sent' => array(
      'label' => t('A report was sent to QuickBooks Online'),
      'module' => 'commerce_qbo_ui',
      'group' => 'Commerce Quickbooks Online',
    ),

    'commerce_qbo_ui_data_import_requested' => array(
      'label' => t('Data was requested from QuickBooks Online by the server'),
      'module' => 'commerce_qbo_ui',
      'group' => 'Commerce Quickbooks Online',
    ),

    'commerce_qbo_ui_data_imported' => array(
      'label' => t('Data was imported from Quickbooks Online successfully'),
      'module' => 'commerce_qbo_ui',
      'group' => 'Commerce Quickbooks Online',
    ),
  );
}

/**
 * Implements hook_rules_action_info()
 *
 * @return array
 */
function commerce_qbo_ui_rules_action_info() {
  return array(
    'commerce_qbo_ui_send_report' => array(
      'label' => t('Send a report to QuickBooks Online'),
      'group' => 'Commerce Quickbooks Online',
      'parameter' => array(
        'order_id' => array(
          'type' => 'commerce_order',
          'label' => t('The order object'),
        ),
        'report_type' => array(
          'type' => 'text',
          'label' => t('The type of report to send'),
          'options list' => 'commerce_qbo_ui_rule_action_report_options',
          'default value' => '',
          'description' => t('This list is populated by Report Types. !link',
            array('!link' => l(t('Create a Report Type.'), 'admin/commerce/commerce_qbo_ui/qb_reports/add'))
          ),
        ),
      ),
      'provides' => array(
        'success_report' => array(
          'type' => 'boolean',
          'label' => t('Report Status'),
        ),
      ),
    ),

    'commerce_qbo_ui_stage_report' => array(
      'label' => t('Stage a report for admin review before sending to QuickBooks Online'),
      'group' => 'Commerce Quickbooks Online',
      'parameter' => array(
        'order_id' => array(
          'type' => 'commerce_order',
          'label' => t('The order object'),
        ),
        'memo' => array(
          'type' => 'text',
          'label' => t('Memo for pending report'),
          'description' => t('Should be a description of what kind of report you want this to be sent as.'),
          'default value' => 'invoice',
        ),
      ),
    ),
  );
}

/**
 * Returns a list of available report type options
 *
 * @return array
 */
function commerce_qbo_ui_rule_action_report_options() {
  $options = array();

  $report_types = db_select('commerce_qbo_ui_report', 'rt')
    ->fields('rt', array('rtid', 'name'))
    ->condition('status', 1, '=')
    ->execute();

  if ($report_types->rowCount()) {
    while($row = $report_types->fetchAssoc()) {
      $options[$row['rtid']] = $row['name'];
    }
  }
  else {
    $options[''] = t('No report types available');
  }

  return $options;
}

/* ------------------------------------------- +
 | Rules Actions                               |
 + ------------------------------------------- */

/**
 * Callback for commerce_qbo_ui_send_report rule action
 *
 * @see commerce_qbo_ui_rules_action_info().
 *
 * Starts a transaction request with QuickBooks if the module and API are set up
 * correctly and valid data has been passed through.
 *
 * @param $order
 *   The commerce order.  Can be the id or the order object itself
 * @param $rtid
 *   The ID of the report type the transaction will be
 *
 * @return bool
 *   TRANSACTION_SUCCESS or TRANSACTION_FAIL
 */
function commerce_qbo_ui_send_report($order, $rtid) {
  // We need to verify that the passed in parameter is the correct type.
  $order_id = commerce_qbo_ui_check_order($order);
  $qb_report = qb_report_load($rtid);

  $success = FALSE;

  // If the API isn't set up then we cannot send any reports.
  if (!variable_get('commerce_qbo_ui_run')) {
    if (user_access('view commerce qbo ui messages')) {
      $message = t('Unable to send a !report report to QuickBooks until the API is fully configured.', array('!report' => $qb_report->name));
      drupal_set_message($message, 'warning', FALSE);
    }

    // Stage the report if the user chose that option.
    if (variable_get('commerce_qbo_ui_stage_premature_reports')) {
      commerce_qbo_ui_stage_report($order, $qb_report->name);
    }
  }
  else if ($order === FALSE) {
    $message = t('Failed to send report.  A valid order object or ID is required, please review your Commerce QBO UI rules.');
    watchdog('commerce_qbo_ui', $message, array(), WATCHDOG_ERROR);
  }
  else {
    module_load_include('inc', 'commerce_qbo_ui', 'includes/commerce_qbo_ui.qbo_api');
    $success = commerce_qbo_ui_start_txn($order_id, $qb_report);

    if ($success) {
      // Log the status.
      commerce_qbo_ui_log_message($qb_report->name . ' was sent successfully!');

      if (user_access('view commerce qbo ui messages')) {
        $message = t('@report was sent successfully!', array('@report' => $qb_report->name));
        drupal_set_message($message, 'commerce_qbo_ui', FALSE);
      }

      // Trigger the commerce_qbo_ui_report_sent event.
      rules_invoke_event('commerce_qbo_ui_report_sent');
    }
    else {
      // Do Something else.
      commerce_qbo_ui_log_message($qb_report->name . ' failed to send.');

      if (user_access('view commerce qbo ui messages')) {
        $message = t('Failed to send @report', array('@report' => $qb_report->name));
        drupal_set_message($message, 'commerce_qbo_ui', FALSE);
      }
    }
  }

  return $success;
}

/**
 * Callback for commerce_qbo_ui_stage_report rule action
 *
 * @see commerce_qbo_ui_rules_action_info().
 *
 * Saves an order for future submission to QuickBooks with the desired
 * transaction format.  If the module is not fully configured, users will be
 * unable to submit reports.
 *
 * @param $order
 * @param $memo
 */
function commerce_qbo_ui_stage_report($order, $memo = '') {
  // We need to verify that the passed in parameter is the correct type.
  $order = commerce_qbo_ui_check_order($order);

  if ($order === FALSE) {
    $message = t('Failed to store order data.  A valid order object or ID is required, please review your Commerce QBO UI rules.');
    watchdog('commerce_qbo_ui', $message, array(), WATCHDOG_ERROR);
  }
  else {
    // Warn the user that reports can only be sent with a configured API
    if (!variable_get('commerce_qbo_ui_run', 0) && user_access('view commerce qbo ui messages')) {
      $message = t('The order has been staged, but you will be unable to send it until the QuickBooks Online API has been fully configured.');
      drupal_set_message($message, 'warning', FALSE);
    }

    // We only want strings to be passed in.
    if (!is_string($memo)) {
      $memo = 'Staged order';
    }

    // Ensures $memo doesn't exceed the maximum character limit.
    if (strlen($memo) > 127) {
      $memo = substr($memo, 0, 123) . '...';
    }

    // Set this order into the staging table.
    db_insert('commerce_qbo_ui_staged_report')
      ->fields(array(
        'order_id' => $order,
        'memo' => $memo,
        'timestamp' => time(),
      ))
      ->execute();

    // Display notification if option is selected.
    if (variable_get('commerce_qbo_ui_alert_pending_reports', 0)) {
      $message = t('There are QuickBooks reports waiting to be processed, !link to manage them.',
        array('!link' => l(t('go to the queue'), 'admin/commerce/commerce_qbo_ui/pending')));
      drupal_set_message($message, 'warning', FALSE);
    }
  }
}
