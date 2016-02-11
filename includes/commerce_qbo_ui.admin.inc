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
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_config($form, &$form_state) {
  commerce_qbo_ui_ready_check();

  // For capturing fail/success messages from QuickBooks Online
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

  // A safety net for transferring reports to staging
  $form['commerce_qbo_ui_stage_premature_reports'] = array(
    '#description' => t('If enabled, any reports that are sent before the module is fully ' .
      'configured will be caught and passed into staging instead.<br><em>You will still ' .
      'be unable to send these reports until configuration is complete.</em>'),
    '#type' => 'radios',
    '#title' => t('Catch reports sent too early'),
    '#options' => array(
      0 => t('No'),
      1 => t('Yes'),
    ),
    '#default_value' => variable_get('commerce_qbo_ui_stage_premature_reports', 1),
    '#required' => TRUE,
  );

  // Warn privileged users when reports are pending action
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

  return system_settings_form($form);
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
 * If the QBO API is missing the neccessary app tokens then the module will not allow reports to be sent
 */
function commerce_qbo_ui_pending_reports($form, $form_state) {
  commerce_qbo_ui_ready_check();

  // Remove and deleted rows
  $trigger = isset($form_state['triggering_element']) ? $form_state['triggering_element'] : NULL;

  if($trigger != NULL) {
    if($trigger['#value'] == 'Remove') {
      $srid = str_replace('commerce_qbo_ui_remove_', '',$trigger['#name']);

      // Convert the ID to an int
      $srid *= 1;

      _commerce_qbo_ui_clear_sr_by_id($srid);
    }
    else if($trigger['#value'] == 'Submit') {
      $srid = str_replace('commerce_qbo_ui_submit_', '',$trigger['#name']);

      // Convert the ID to an int
      $srid *= 1;

      $result = db_select('commerce_qbo_ui_staged_report', 'sr')
        ->fields('sr', array('order_id'))
        ->condition('rid', $srid, '=')
        ->execute()
        ->fetchAssoc();

      $order_id = $result['rid'];
      $report_type = $form_state['values']['commerce_qbo_ui_report_types'];

      commerce_qbo_ui_send_report($order_id, $report_type);
    }
  }

  $rows = array();

  $reports_pending = db_select('commerce_qbo_ui_staged_report', 'sr')
    ->fields('sr', array('rid', 'order_id', 'memo'))
    ->condition('deleted', 0, '=')
    ->execute();

  $report_types = db_select('commerce_qbo_ui_report_types', 'rt')
      ->fields('rt', array('rtid', 'name'))
      ->condition('active', 1, '=')
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
 *
 */
function commerce_qbo_ui_reports_builder() {

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

  foreach($result as $qbo_log) {
    $rows[] = array('data' =>
      array(
        // Cells
        array('class' => 'icon'),
        format_date($qbo_log->timestamp, 'short'),
        $qbo_log->message,
      ),
      // Attributes for tr
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
