<?php

/**
 * Callback for module settings page.
 */
function media_telvue_connect_importer_settings_form($form, &$form_state) {
  $form = array();

  $form['media_telvue_connect_importer_connect_mrss'] = array(
    '#type' => 'textfield',
    '#title' => t('Connect MRSS XML Source'),
    '#description' => 'i.e., https://connect.telvue.com/mrss/f3efhg7XsR-YcpRlQOnWV8S-4_LF2TVI?recent=50',
    '#default_value' => variable_get('media_telvue_connect_importer_connect_mrss', ''),
    '#size' => 120,
  );

  // Sync mode
  $sync_mode_options = array(
    'basic' => 'Basic (This will create new shows in Drupal from your Connect system. You cannot update existing shows)',
    'advanced' => 'Advanced (This will update existing shows in Drupal that have field_expected_filename filled out.)'
  );

  $form['media_telvue_connect_importer_sync_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Sync Mode'),
    '#options' => $sync_mode_options,
    '#default_value' => variable_get('media_telvue_connect_importer_sync_mode', 'basic'),
    '#description' => '',
    '#size' => 120,
  );

  return system_settings_form($form);
}
