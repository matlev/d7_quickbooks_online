<?php

/**
 * @file The controller class for a QuickBooks Report entity
 */

class QBOReportEntityController extends EntityAPIController {

  /**
   * Create a default qb_report.
   *
   * @param array $values
   *   An array of values to set, keyed by property name.
   *
   * @return object
   *   A qb_report object with all default fields initialized.
   */
  public function create(array $values = array()) {
    $values += array(
      'is_new' => TRUE,
      'rtid' => NULL,
      'name' => '',
      'status' => 1,
      'created' => '',
      'changed' => ''
    );

    return parent::create($values);
  }

  /**
   * Saves a qb_report.
   *
   * @param $report
   *   The full qb_report object to save.
   * @param $transaction
   *   An optional transaction object.
   *
   * @return
   *   SAVED_NEW or SAVED_UPDATED depending on the operation performed.
   */
  public function save($report, DatabaseTransaction $transaction = NULL) {
    global $user;

    // Hardcode the changed time.
    $report->changed = REQUEST_TIME;

    if (empty($report->{$this->idKey}) || !empty($report->is_new)) {
      // Set the creation timestamp if not set, for new entities.
      if (empty($report->created)) {
        $report->created = REQUEST_TIME;
      }
    }
    else {
      if ($report->created === '') {
        unset($report->created);
      }
    }

    // Determine if we will be inserting a new qb_report.
    $report->is_new = empty($report->rtid);

    return parent::save($report, $transaction);
  }

  /**
   * Deletes multiple qb_reports by ID.
   *
   * @param $report_ids
   *   An array of qb_report IDs to delete.
   * @param $transaction
   *   An optional transaction object.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($report_ids, DatabaseTransaction $transaction = NULL) {
    try {
      parent::delete($report_ids, $transaction);
      return TRUE;
    }
    catch (Exception $e) {
      return FALSE;
    }
  }

  /**
   * Builds a structured array representing the entity's content.
   *
   * This entity only has the 'full' view mode
   *
   * @param $report
   *   An entity object.
   * @param $view_mode
   *   View mode, e.g. 'full', 'teaser'...
   * @param $langcode
   *   (optional) A language code to use for rendering. Defaults to the global
   *   content language of the current request.
   * @param array $content
   *
   * @return array
   *   The renderable array.
   */
  public function buildContent($report, $view_mode = 'full', $langcode = NULL, $content = array()) {

    // @TODO: Configure build content here if needed

    $build = parent::buildContent($report, $view_mode, $langcode, $content);

    return $build;

  }
}