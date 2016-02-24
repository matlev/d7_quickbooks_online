<?php

/**
 * @file
 * Page callbacks and form builder functions for administering QuickBooks Report.
 */

/**
 * Menu callback: display a list of qb_report types that the user can create.
 */
function commerce_qbo_ui_qb_report_add_page() {
  $item = menu_get_item();
  $content = system_admin_menu_block($item);

  return theme('commerce_qbo_ui_qb_report_add_list', array('content' => $content));
}

/**
 * Displays the list of available product types for product creation.
 *
 * @param $variables
 * @return String
 *
 * @ingroup themeable
 */
function theme_commerce_qbo_ui_qb_report_add_list($variables) {
  $content = $variables['content'];
  $output = '';

  $output = '<dl class="commerce-qbo-ui-qb-report-type-list">';
  foreach ($content as $item) {
    $output .= '<dt>' . l($item['title'], $item['href'], $item['localized_options']) . '</dt>';
    $output .= '<dd>' . filter_xss_admin($item['description']) . '</dd>';
  }
  $output .= '</dl>';

  return $output;
}

/**
 * A form wrapper to customize the qb_report form
 *
 * @param $report
 * @return array|mixed
 */
function commerce_qbo_ui_qb_report_form_wrapper($report) {
  // Include the forms file from the Product module.
  module_load_include('inc', 'commerce_qbo_ui', 'includes/commerce_qbo_ui.forms');
  return drupal_get_form('commerce_qbo_ui_qb_report_form', $report);
}

/**
 * A form wrapper to return a confirmation form for deleting a product
 *
 * @param $report
 * @return array|mixed
 */
function commerce_qbo_ui_qb_report_delete_form_wrapper($report) {
  // Include the forms file from the Product module.
  module_load_include('inc', 'commerce_qbo_ui', 'includes/commerce_qbo_ui.forms');
  return drupal_get_form('commerce_qbo_ui_qb_report_delete_form', $report);
}