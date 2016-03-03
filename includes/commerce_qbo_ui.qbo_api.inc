<?php
/**
 * @file Contains all functions for making calls to and from Quickbooks
 */

/* ------------------------------------------- +
 | Definitions                                 |
 + ------------------------------------------- */

/**
 * Callback value to inform the module of a successful transaction
 */
define('TRANSACTION_SUCCESS', 1);

/**
 * Callback value to inform the module of a failed transaction
 */
define('TRANSACTION_FAILURE', 0);

/* ------------------------------------------- +
 | Main Functions                              |
 + ------------------------------------------- */

/**
 * Connects to QuickBooks Online and begins the transaction process
 *
 * Will return false if the requested transaction type isn't supported by the
 * current library, a connection cannot be made to the QuickBooks Online client,
 * or if a transaction fails.
 *
 * @param $order
 * @param $qb_report
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_start_txn($order_id, $qb_report) {
  // Retrieve the proper function to use for building the transaction
  $txn = commerce_qbo_ui_route_txn($qb_report);
  $func = "commerce_qbo_ui_$txn";

  // If the report type doesn't have an associated library class yet then we
  // can't process this transaction.  This should only occur if a module defines
  // a bundle with a mis-formatted name or one that doesn't match a transaction
  // type, or QuickBooks has defined a new transaction type but the current qbo
  // library is out of date.
  if(!function_exists($func)) {
    $message = t('Unable to process transaction request: @func does not exist.', array('@func' => $txn));
    commerce_qbo_ui_log_message($message);
    return TRANSACTION_FAILURE;
  }

  // Create a connection to QuickBooks and pass the data service by reference to
  // the transaction functions, or return false if the connection fails.
  try {
    $client = qbo_api_client();
    if($client === NULL) {
      $error = t('Missing or corrupt tokens, please verify API tokens in qbo_api module settings.');
      throw new Exception($error);
    }
  }
  catch (Exception $e) {
    // We watchdog the error in this case because this is considered to be a
    // critical failure and the site admin shouldn't miss it if it happens
    $message = t('Couldn\'t connect to QuickBooks.  Verify that your API tokens are correct, your connection is secure and all services are running. ');
    watchdog('commerce_qbo_ui', $message . $e->getMessage(), array(), WATCHDOG_CRITICAL);
    commerce_qbo_ui_log_message($message, $e);
    return TRANSACTION_FAILURE;
  }

  // Retrieve the order
  watchdog('commerce_qbo_ui', print_r($order_id, TRUE));
  $order = commerce_order_load($order_id);

  // Call the defined transaction method
  libraries_load('qbo');
  return $func($order, $qb_report, $txn, $client);
}

/**
 * Requests the relevant function for processing this transaction
 *
 * @param $qb_report
 *   The qb_report entity associated with the current order
 *
 * @return string
 *   The function to call for building a transaction
 */
function commerce_qbo_ui_route_txn($qb_report) {
  $name = commerce_qbo_ui_get_qb_report_type($qb_report->rtid);

  // Make sure all parts of the name are capitalized, then strip spaces
  $name = str_replace(' ', '', ucwords($name));

  return 'IPP' . $name;
}

/* ------------------------------------------- +
 | Transaction Functions                       |
 + ------------------------------------------- */

/**
 * Builds and submits a Credit Memo transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPCreditMemo($order, $qb_report, $txn_class, &$client) {
// Initialize the transaction object
  $txn = new IPPCreditMemo();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Initialize Line array
  $txn->Line = array();
  $success = commerce_qbo_ui_build_txn_line($order, $qb_report, $txn);
  if(!$success) {
    return TRANSACTION_FAILURE;
  }

  // Pass in the non-mandatory values now
  $ewr = entity_metadata_wrapper('qb_report', $qb_report);
  $txn->PrivateNote = $ewr->commerce_qbo_ui_memo->value();
  $txn->CustomerMemo = $ewr->commerce_qbo_ui_message->value();
  $txn->PONumber = $ewr->commerce_qbo_ui_po_number->value();

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/**
 * Builds and submits an Estimate transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPEstimate($order, $qb_report, $txn_class, &$client) {
// Initialize the transaction object
  $txn = new IPPEstimate();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Initialize Line array
  $txn->Line = array();
  $success = commerce_qbo_ui_build_txn_line($order, $qb_report, $txn);
  if(!$success) {
    return TRANSACTION_FAILURE;
  }

  // Pass in the non-mandatory values now
  // @TODO: Fill out this section

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/**
 * Builds and submits an Invoice transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPInvoice($order, $qb_report, $txn_class, &$client) {
  // Initialize the transaction object
  $txn = new IPPInvoice();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Initialize Line array
  $txn->Line = array();
  $success = commerce_qbo_ui_build_txn_line($order, $qb_report, $txn);
  if(!$success) {
    return TRANSACTION_FAILURE;
  }

  // Pass in the non-mandatory values now
  // @TODO: Fill out this section

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/**
 * Builds and submits a Payment transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPPayment($order, $qb_report, $txn_class, &$client) {
  $ewo = entity_metadata_wrapper('comerce_order', $order);
  $ewr = entity_metadata_wrapper('qb_report', $qb_report);

  // Initialize the transaction object
  $txn = new IPPPayment();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Amount
  $txn->TotalAmt = $ewo->commerce_order_total->amount_decimal->value();

  // Payment method
  $payment_method = _commerce_qbo_ui_get_payment_method($ewo);
  $txn->PaymentMethodRef = commerce_qbo_ui_get_payment_method_ref($payment_method);

  // Deposit account
  if($ewr->commerce_qbo_ui_deposit_account->value() == NULL) {
    $deposit_account_ref = $ewr->commerce_qbo_ui_deposit_acc_s->value();
  }
  else {
    $deposit_account_ref = $ewr->commerce_qbo_ui_deposit_account->value();
  }
  $txn->DepositToAccountRef = $deposit_account_ref;

  // Memo
  //$txn->PrivateNote = $ewr->commerce_qbo_ui_memo->value();

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/**
 * Builds and submits a Refund transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPRefund($order, $qb_report, $txn_class, &$client) {
  // Initialize the transaction object
  $txn = new IPPRefundReceipt();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Initialize Line array
  $txn->Line = array();
  $success = commerce_qbo_ui_build_txn_line($order, $qb_report, $txn);
  if(!$success) {
    return TRANSACTION_FAILURE;
  }

  // Pass in the non-mandatory values now
  // @TODO: Fill out this section

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/**
 * Builds and submits a Sales Receipt transaction
 *
 * @param $order
 * @param $qb_report
 * @param $txn_class
 * @param $client
 *
 * @return bool
 *   TRANSACTION_SUCCESS on successful connection and processing,
 *   TRANSACTION_FAILURE otherwise
 */
function commerce_qbo_ui_IPPSalesReceipt($order, $qb_report, $txn_class, &$client) {
// Initialize the transaction object
  $txn = new IPPSalesReceipt();

  // Attach the customer to the transaction
  $txn->CustomerRef = commerce_qbo_ui_get_customer_id($order);
  if($txn->CustomerRef == FALSE) {
    return TRANSACTION_FAILURE;
  }

  // Initialize Line array
  $txn->Line = array();
  $success = commerce_qbo_ui_build_txn_line($order, $qb_report, $txn);
  if(!$success) {
    return TRANSACTION_FAILURE;
  }

  // Pass in the non-mandatory values now
  // @TODO: Fill out this section

  // Finally, attempt to pass the transaction to QuickBooks
  try {
    $txn = $client->Add($txn);

    return !empty($txn) ? TRANSACTION_SUCCESS : TRANSACTION_FAILURE;
  }
  catch (Exception $e) {
    // The operation failed..
    commerce_qbo_ui_log_message("Failed to send the $txn_class transaction to QuickBooks.", $e);
    return TRANSACTION_FAILURE;
  }
}

/* ------------------------------------------- +
 | Helper Functions                            |
 + ------------------------------------------- */

/**
 * Builds the Line data for a QBO transaction
 *
 * @param $order
 *   The order object
 * @param $qb_report
 *   The qb_report object
 * @param $txn
 *   Transaction object
 */
function commerce_qbo_ui_build_txn_line($order, $qb_report, &$txn) {
  // Wrap the order and report
  $ewo = entity_metadata_wrapper('commerce_order', $order);
  $ewr = entity_metadata_wrapper('qb_report', $qb_report);

  // Loop through the commerce line items, and build the Line according to the
  // qb_report configuration
  foreach($ewo->commerce_line_items->getIterator() as $line_item_wrapper) {
    // Get the regular commerce_line_item object
    $line_item = commerce_line_item_load($line_item_wrapper->line_item_id->value());
    $line_item_type = $line_item_wrapper->type->value();

    foreach($ewr->c_qbo_ui_line_item_collection->getIterator() as $qb_collection) {
      if($qb_collection->c_qbo_ui_line_item_type->value() == $line_item_type) {
        // Replace tokens in product/service and description fields
        $data = array('commerce-order' => $order, 'commerce-line-item' => $line_item);
        $parsed_text = _commerce_qbo_ui_replace_tokens($qb_collection, $data);

        // Build the line
        $row = new IPPLine();
        $row->DetailType = 'SalesItemLineDetail';
        $row->SalesItemLineDetail = new IPPSalesItemLineDetail();

        // Item Ref
        $item_ref = _commerce_qbo_ui_get_item_ref($parsed_text['item_name'], $parsed_text['description'], $line_item_wrapper, $qb_collection);
        if($item_ref === FALSE) {
          // Item ref is a required field, we can't continue without it
          return FALSE;
        }
        $row->SalesItemLineDetail->ItemRef = $item_ref;

        // Quantity
        if($qb_collection->c_qbo_ui_line_quantity->value() != NULL) {
          $row->SalesItemLineDetail->Qty = $qb_collection->c_qbo_ui_line_quantity->value();
        }
        else if($line_item_wrapper->quantity->value() != NULL) {
          $row->SalesItemLineDetail->Qty = intval($line_item_wrapper->quantity->value());
        }
        else {
          $row->SalesItemLineDetail->Qty = 0;
        }

        // Unit Price
        if($qb_collection->c_qbo_ui_line_rate->value() != NULL) {
          $row->SalesItemLineDetail->UnitPrice = $qb_collection->c_qbo_ui_line_rate->value();
        }
        else if($line_item_wrapper->commerce_unit_price->amount_decimal->value() != NULL) {
          $row->SalesItemLineDetail->UnitPrice = $line_item_wrapper->commerce_unit_price->amount_decimal->value();
        }
        else {
          $row->SalesItemLineDetail->UnitPrice = 0;
        }

        // Amount
        $row->Amount = $row->SalesItemLineDetail->UnitPrice * $row->SalesItemLineDetail->Qty;

        // Tax Code Ref
        // @TODO: Distinguish between Global and US account

        // Description
        if(!empty($qb_collection->c_qbo_ui_line_description->value())) {
          $row->Description = $parsed_text['description'];
        }

        $txn->Line[] = $row;
      }
    }
  }

  // Success flag
  return TRUE;
}

/**
 * Queries for the payment method used for the given order
 *
 * @param $order
 *   The wrapped commerce_order entity
 *
 * @return string
 *   The payment method
 */
function _commerce_qbo_ui_get_payment_method($order) {
  $payment_method = '';

  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'commerce_payment_transaction')
    ->propertyCondition('order_id', $order->order_id->value());

  $results = $query->execute();
  if (!empty($results['commerce_payment_transaction'])) {
    $transactions = entity_load('commerce_payment_transaction', array_keys($results['commerce_payment_transaction']));
    foreach ($transactions as $transaction) {
      $payment_method = $transaction->payment_method;
    }
  }

  return $payment_method;
}

/**
 * Replaces the tokens in qb_report fields with text values
 *
 * Internal function only, should not be called outside of this file.
 *
 * @param $qb_collection
 *   qb_report entity wrapper
 * @param array $data
 *   The objects to reference token values from
 *
 * @return array
 *   The field values with possible token replacements
 */
function  _commerce_qbo_ui_replace_tokens($qb_collection, $data = array()) {
  $return = array();

  $text = $qb_collection->c_qbo_ui_product_service->value();
  $return['item_name'] = token_replace($text, $data);

  $text = $qb_collection->c_qbo_ui_line_description->value();
  $return['description'] = token_replace($text, $data);

  return $return;
}

/**
 * Returns the ref. ID of an Item if it exists
 *
 * @param $item_name
 *   The name of the Item to reference in the Line
 * @param $description
 *   The description of the Item
 * @param $line_item
 *   Passed through in case a new Item needs to be created
 * @param $qb_collection
 *   Passed through in case a new Item needs to be created
 *
 * @return bool|\com\intuit\schema\finance\v3\IPPid
 *   FALSE if no item existed and we couldn't create one, an ID otherwise
 */
function _commerce_qbo_ui_get_item_ref($item_name, $description, $line_item, $qb_collection) {
  $item_name = check_plain($item_name);
  $query = "SELECT Id FROM Item WHERE Name = '$item_name'";
  $items = qbo_api_query($query, TRUE);

  if(count($items)) {
    foreach($items as $item) {
      $id = $item->Id;
    }

    return $id;
  }
  else {
    // No item exists by the given name, try to create one and pass it back
    $item = _commerce_qbo_ui_create_item($item_name, $description, $line_item, $qb_collection);

    return $item ? $item->Id : FALSE;
  }
}

/**
 * Creates a Service Item in Quickbooks
 *
 * @TODO: Add check for user-given permission to create an entity
 *
 * @param $item_name
 *   The name of the Item to reference in the Line
 * @param $description
 *   The description of the Item
 * @param $line_item
 *   Passed through in case a new Item needs to be created
 * @param $qb_collection
 *   Passed through in case a new Item needs to be created
 *
 * @return bool|IPPIntuitEntity
 *   A new Item or FALSE if we failed to create one
 */
function _commerce_qbo_ui_create_item($item_name, $description, $line_item, $qb_collection) {
  $client = qbo_api_client();

  $new_item = new IPPItem();
  $new_item->Name = check_plain($item_name);
  $new_item->Type = "Service";
  $new_item->Description = $description;

  // Calculate Unit price for this item
  if(!empty($qb_collection->c_qbo_ui_line_rate->value())) {
    $new_item->UnitPrice = $qb_collection->c_qbo_ui_line_rate->value();
  }
  else {
    $new_item->UnitPrice = $line_item->commerce_unit_price->amount_decimal->value();
  }

  // Get the default account to deposit funds from the sale of this Item to
  $account_ref = qbo_api_query("SELECT Id FROM Account WHERE Name = 'Sales of Product Income'");
  $new_item->IncomeAccountRef = $account_ref[0]->Id;

  // Get the default payable account for the vendor that supplies this Item
  $account_ref = qbo_api_query("SELECT Id FROM Account WHERE Name = 'Cost of Goods Sold'");
  $new_item->ExpenseAccountRef = $account_ref[0]->Id;

  try {
    $item = $client->Add($new_item);
    return $item;
  }
  catch (Exception $e) {
    commerce_qbo_ui_log_message("Unable to create new item [" . $item_name . "]. ", $e);
    return FALSE;
  }
}

/**
 * Returns the ref ID of a Customer if they exist
 *
 * @param $order
 *   The commerce_order object
 *
 * @return bool|\com\intuit\schema\finance\v3\IPPid
 *   FALSE if no Customer existed and we couldn't create one, an ID otherwise
 */
function commerce_qbo_ui_get_customer_id($order) {
  // Wrap the order and report
  $ewo = entity_metadata_wrapper('commerce_order', $order);

  $customer_name = check_plain($ewo->commerce_customer_billing->commerce_customer_address->name_line->value());
  $email = check_plain($ewo->commerce_customer_billing->user->mail->value());

  $query = "SELECT Id FROM Customer WHERE DisplayName = '$customer_name'";
  $customers = qbo_api_query($query, TRUE);

  if(count($customers)) {
    foreach($customers as $customer) {
      $id = $customer->Id;
    }

    return $id;
  }
  else {
    // Try a new query against email instead.  Unfortunately QuickBooks doesn't
    // play well with OR statements in the SQL, so separate queries are necessary
    $query = "SELECT Id FROM Customer WHERE PrimaryEmailAddr = '$email'";
    $customers = qbo_api_query($query, TRUE);

    if(count($customers)) {
      foreach($customers as $customer) {
        $id = $customer->Id;
      }

      return $id;
    }
    else {
      // No item exists by the given name, try to create one and pass it back
      $customer = _commerce_qbo_ui_create_customer($customer_name, $ewo);

      return $customer ? $customer->Id : FALSE;
    }
  }
}

/**
 * Creates a Customer in QuickBooks
 *
 * @TODO: Add check for user-given permission to create an entity
 *
 * @param $customer_name
 *   The full name (First and Last)
 * @param $ewo
 *   The commerce_order entity wrapper
 *
 * @return bool|IPPIntuitEntity
 *   A new Customer or FALSE if we failed to create one
 */
function _commerce_qbo_ui_create_customer($customer_name, $ewo) {
  $client = qbo_api_client();

  // Main identifiers
  $new_customer = new IPPCustomer();
  $new_customer->DisplayName = $customer_name;
  $new_customer->PrimaryEmailAddr = new IPPEmailAddress();
  $new_customer->PrimaryEmailAddr->Address = check_plain($ewo->commerce_customer_billing->user->mail->value());

  // Address (Minimal, Country, State/Province and Postal Code)
  $new_customer->BillAddr = new IPPPhysicalAddress();
  $new_customer->BillAddr->Line1 = $ewo->commerce_customer_billing->commerce_customer_address->thoroughfare->value();
  $new_customer->BillAddr->Line2 = $ewo->commerce_customer_billing->commerce_customer_address->premise->value();
  $new_customer->BillAddr->Country = $ewo->commerce_customer_billing->commerce_customer_address->country->value();
  $new_customer->BillAddr->CountrySubDivisionCode = $ewo->commerce_customer_billing->commerce_customer_address->administrative_area->value();
  $new_customer->BillAddr->City = $ewo->commerce_customer_billing->commerce_customer_address->locality->value();
  $new_customer->BillAddr->PostalCode = $ewo->commerce_customer_billing->commerce_customer_address->postal_code->value();

  // Shipping Address
  $new_customer->ShipAddr = new IPPPhysicalAddress();
  $new_customer->ShipAddr->Line1 = $ewo->commerce_customer_shipping->commerce_customer_address->thoroughfare->value();
  $new_customer->ShipAddr->Line2 = $ewo->commerce_customer_shipping->commerce_customer_address->premise->value();
  $new_customer->ShipAddr->Country = $ewo->commerce_customer_shipping->commerce_customer_address->country->value();
  $new_customer->ShipAddr->CountrySubDivisionCode = $ewo->commerce_customer_shipping->commerce_customer_address->administrative_area->value();
  $new_customer->ShipAddr->City = $ewo->commerce_customer_shipping->commerce_customer_address->locality->value();
  $new_customer->ShipAddr->PostalCode = $ewo->commerce_customer_shipping->commerce_customer_address->postal_code->value();

  try {
    $customer = $client->Add($new_customer);
    return $customer;
  }
  catch (Exception $e) {
    commerce_qbo_ui_log_message("Unable to create new customer [" . $customer_name . "]. ", $e);
    return FALSE;
  }
}

/**
 * Returns the ref ID of a payment method if it exists
 *
 * @param $payment_method
 *   Name of the Payment Method
 *
 * @return \com\intuit\schema\finance\v3\IPPid|string
 *   Blank if no method exists and we couldn't create one, the ref ID otherwise
 */
function commerce_qbo_ui_get_payment_method_ref($payment_method) {
  $query = "SELECT Id FROM PaymentMethod WHERE Name = '" . check_plain($payment_method) . "'";

  $payment_methods = qbo_api_query($query, TRUE);

  if(count($payment_methods)) {
    foreach($payment_methods as $method) {
      $id = $method->Id;
    }

    return $id;
  }
  else {
    // No method exists by the given name, try to create one and pass it back
    $method = _commerce_qbo_ui_create_payment_method($payment_method);

    // This is not a required field so it's ok to pass back a blank value
    return $method ? $method->Id : '';
  }
}

/**
 * Creates a Payment Method in QuickBooks
 *
 * @param $payment_method
 *   Name of the Method to create
 *
 * @return bool|IPPIntuitEntity
 *   A new Payment Method or FALSE if we failed to create one
 */
function _commerce_qbo_ui_create_payment_method($payment_method) {
  $client = qbo_api_client();

  $new_payment_method = new IPPPaymentMethod();
  $new_payment_method->Name = $payment_method;

  try {
    $method = $client->Add($new_payment_method);
    return $method;
  }
  catch (Exception $e) {
    commerce_qbo_ui_log_message("Unable to create new customer [" . $payment_method . "]. ", $e);
    return FALSE;
  }
}
