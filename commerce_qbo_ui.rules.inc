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
          'label' => 'The order object',
        ),
        'report_type' => array(
          'type' => 'text',
          'label' => t("The type of report to send"),
          'options list' => 'commerce_qbo_ui_rule_action_report_options',
          'default value' => '',
          'description' => t('This list is populated by Report Types. !link',
            array('!link' => l(t('Create a Report Type.'), "admin/commerce/commerce_qbo_ui/report_types"))
          ),
        ),
      ),
      'provides' => array(
        'success_report' => array(
          'type' => 'boolean',
          'label' => 'Report Status',
        ),
      ),
    ),

    'commerce_qbo_ui_stage_report' => array(
      'label' => t('Stage a report for admin review before sending to QuickBooks Online'),
      'group' => 'Commerce Quickbooks Online',
      'parameter' => array(
        'order_id' => array(
          'type' => 'commerce_order',
          'label' => 'The order object',
        ),
        'memo' => array(
          'type' => 'text',
          'label' => 'Memo for pending report',
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
    ->condition('active', 1, '=')
    ->execute();

  if($report_types->rowCount()) {
    while($row = $report_types->fetchAssoc()) {
      $options[$row['rtid']] = $row['name'];
    }
  }
  else {
    $options[''] = t('No report types available');
  }

  return $options;
}
