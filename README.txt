CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Requirements
 * Installation
 * Configuration
 * Troubleshooting
 * FAQ
 * Maintainers


INTRODUCTION
------------

The Commerce QuickBooks Online UI module provides an interface to set up when,
how and what kind of data gets sent to their QuickBooks Online account.

 * For a full description of the module, visit the project page:
   https://www.drupal.org/sandbox/mlevasseur/2663770

 * To submit bug reports and feature suggestions, or to track changes:
   https://www.drupal.org/project/issues/2663770


REQUIREMENTS
------------

This module requires the following modules:

 * Quickbooks Online API (https://www.drupal.org/project/qbo_api)
 * Drupal Commerce (https://www.drupal.org/project/commerce)
 * Rules (https://www.drupal.org/project/rules)
 * Date (https://www.drupal.org/project/date)
 * Entity API (https://www.drupal.org/project/entity)
 * Entity reference (https://www.drupal.org/project/entityreference)
 * Field collection (https://www.drupal.org/project/field_collection)


INSTALLATION
------------

 * Install as you would normally install a contributed Drupal module. See:
   https://drupal.org/documentation/install/modules-themes/modules-7
   for further information.


CONFIGURATION
-------------

This module relies on the Quickbooks Online API module to function properly.
Before configuring this module, make sure you have enabled and followed all
configuration steps for Quickbooks Online API.  A warning will be displayed
that this module will be unusable until this step is completed.

 * Do basic setup and import some basic QuickBooks data (optional, but
   recommended) in Administration » Store » Quickbooks » Config

   - Populate some entity fields with data from Quickbooks

     This will allow you to select things like what kind of account a report
     should withdraw or deposit funds into, and map your QuickBooks tax types
     to your Drupal tax rates.

 * Configure your first report type in Administration » Store » Quickbooks »
   Report Types » Select a Report Type

   - Choose a report type to configure.

     Report types act as templates to be used for structuring the request to
     QuickBook when a report is being generated.  Some fields accept tokens
     for true templating capabilities.

     For report types with configurable QuickBooks Line Items, each line item
     produces one-to-many lines in your QuickBooks report.  Example: a Line
     set to be created for Product type line items will generate a line in the
     report for each Product that appears in the order.

 * Set up a rule for when a report should be generated in Administration »
   Store » Quickbooks

   - Click "Add a QuickBooks Report Rule"

     Follow the standard Rule setup process, choosing what even should trigger
     sending a report, selecting a report to be sent and if it should be sent
     immediately or stored for review and manual export.  Note: you will be
     unable to create a rule if you haven't set up at least one report type.


TROUBLESHOOTING
---------------

 * The module fails to create a user or item

   - This is generally a sign that your site has disconnect from Intuit.  Try
     reconnecting and triggering another report.

 * The module fails to send reports completely

   - This module is written to function with a custom namespaced version of the
     QBO PHP v3 SDK, in order to address collision issues when popular modules
     like Message are installed.  If you are not using the namespaced version,
     you will have to remove the '\Intuit\IPP\Data\' prefix anywhere it's used
     in /includes/commerce_qbo_ui.qbo_api.inc.


FAQ
---

Q: Are more reports types going to be supported in the future?

A: Only if they're requested a lot.  A future upgrade to the module will allow
   developers to add their own report type entities and include the report
   generation logic to go with them through hooks.


MAINTAINERS
-----------

Current maintainers:
 * Mathew Levasseur (mlevasseur) - https://www.drupal.org/u/mlevasseur

This project has been sponsored by:
 * ACRO MEDIA INC - https://www.drupal.org/acro-media-inc
   Acro Media is a Drupal Commerce development agency that specializes in
   enterprise-level ecommerce. We work with industry leaders looking to take
   that next step up. We are committed to building strong strategic partnerships
   and using our ecommerce expertise to help clients create a dynamic web
   presence that engages audiences, generates revenue, and boosts brand awareness.
