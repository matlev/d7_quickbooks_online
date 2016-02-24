<?php

/**
 * @file
 * Forms for creating, editing, and deleting QuickBooks reports.
 */


/**
 * Form callback: create or edit a qb_report.
 *
 * @param $form
 * @param $form_state
 * @param $report
 *   The report object to edit or for a create form an empty report object
 *     with only a report type defined.
 *
 * @return array
 */
function commerce_qbo_ui_qb_report_form($form, &$form_state, $report) {
  // Ensure this include file is loaded when the form is rebuilt from the cache.
  $form_state['build_info']['files']['form'] = drupal_get_path('module', 'commerce_qbo_ui') . '/includes/commerce_qbo_ui.forms.inc';

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $report->name,
    '#maxlength' => 255,
    '#required' => TRUE,
    '#weight' => -5,
  );

  // Add the field related form elements.
  $form_state['qb_report'] = $report;

  field_attach_form('qb_report', $report, $form, $form_state, LANGUAGE_NONE);

  $form['status'] = array(
    '#type' => 'radios',
    '#title' => t('Status'),
    '#description' => t('Disabled reports cannot be selected or used when generating a QuickBooks Report.'),
    '#options' => array(
      '1' => t('Active'),
      '0' => t('Disabled'),
    ),
    '#default_value' => $report->status,
    '#required' => TRUE,
    '#weight' => 35,
  );

  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 400,
  );

  // We add the form's #submit array to this button along with the actual submit
  // handler to preserve any submit handlers added by a form callback_wrapper.
  $submit = array();

  if (!empty($form['#submit'])) {
    $submit += $form['#submit'];
  }

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save report'),
    '#submit' => array_merge($submit, array('commerce_qbo_ui_qb_report_form_submit')),
  );

  // We append the validate handler to #validate in case a form callback_wrapper
  // is used to add validate handlers earlier.
  $form['#validate'][] = 'commerce_qbo_ui_qb_report_form_validate';

  return $form;
}

/**
 * Validation callback for commerce_qbo_ui_qb_report_form().
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_qb_report_form_validate($form, &$form_state) {
  // Notify field widgets to validate their data.
  $report = $form_state['qb_report'];
  field_attach_form_validate('qb_report', $report, $form, $form_state);
}

/**
 * submit callback for commerce_qbo_ui_qb_report_form().
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_qb_report_form_submit($form, &$form_state) {
  $report = $form_state['qb_report'];

  // Set default entity values
  $report->name = $form_state['values']['name'];
  $report->status = $form_state['values']['status'];

  // Notify field widgets.
  field_attach_submit('qb_report', $report, $form, $form_state);

  // Save the product.
  commerce_qbo_ui_qb_report_save($report);

  // Redirect back to reports listing page
  $form_state['redirect'] = 'admin/commerce/commerce_qbo_ui/qb_reports';
  drupal_set_message(t('QuickBooks Report saved.'));
}

/**
 * Form callback: confirmation form for deleting a qb_report.
 *
 * @param $report
 *   The qb_report object to be deleted.
 *
 * @see confirm_form()
 */
function commerce_qbo_ui_qb_report_delete_form($form, &$form_state, $report) {
  $form_state['qb_report'] = $report;

  // Ensure this include file is loaded when the form is rebuilt from the cache.
  $form_state['build_info']['files']['form'] = drupal_get_path('module', 'commerce_qbo_ui') . '/includes/commerce_qbo_ui.forms.inc';

  $form['#submit'][] = 'commerce_qbo_ui_qb_report_delete_form_submit';

  $content = entity_view('qb_report', array($report->rtid => $report));

  $form = confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $report->name)),
    '',
    drupal_render($content) . '<p>' . t('Deleting this report cannot be undone.') . '</p>',
    t('Delete'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Submit callback for commerce_qbo_ui_qb_report_delete_form().
 *
 * @param $form
 * @param $form_state
 */
function commerce_qbo_ui_qb_report_delete_form_submit($form, &$form_state) {
  $report = $form_state['qb_report'];

  if (commerce_qbo_ui_qbreport_delete($report->rtid)) {
    drupal_set_message(t('%name has been deleted.', array('%name' => $report->name)));
    watchdog('qb_report', 'Deleted qb_report %name.', array('%name' => $report->name), WATCHDOG_NOTICE);
    $form_state['redirect'] = 'admin/commerce/commerce_qbo_ui/qb_reports';
  }
  else {
    drupal_set_message(t('%name could not be deleted.', array('%name' => $report->name)), 'error');
  }
}