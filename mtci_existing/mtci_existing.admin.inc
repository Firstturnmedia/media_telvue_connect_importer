<?php

/**
 * Callback for module settings page.
 */
function mtci_existing_settings_form($form, &$form_state) {
  $form = array();

  $form['mtci_existing_connect_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Connect ID'),
    '#description' => 'i.e., f3efhg7XsR-YcpRlQOnWV8S-4_LF2TVI',
    '#default_value' => variable_get('mtci_existing_connect_id', ''),
    '#size' => 120,
  );

  return system_settings_form($form);
}
