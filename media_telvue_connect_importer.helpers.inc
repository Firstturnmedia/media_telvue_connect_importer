<?php

/**
 * Helper function to get xml source.
 */
function media_telvue_connect_importer_get_xml_source() {
  $url = MTCI_CONNECT_MRSS;

  // Init curl and pull in rss feed.
  $curl = curl_init();
  curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($curl, CURLOPT_URL, $url);
  $xml = curl_exec($curl);

  // Close curl
  curl_close($curl);

  return $xml;
}

/**
 * Helper function to parse xml source.
 */
function media_telvue_connect_importer_parse_xml_source() {
  $xml = media_telvue_connect_importer_get_xml_source();

  $channel = new SimpleXMLElement($xml);
  $namespaces = $channel->getNamespaces(true);

  foreach($channel->xpath('//item') as $item) {
    // Get data in cloudcast: namespace
    $cloudcast = $item->children($namespaces['cloudcast']);
    // Get data in media: namespace.
    $media = $item->children($namespaces['media']);

    $data[] = array(
      'link' => (string) $item->{'link'},
      'title' => (string) $cloudcast->summary,
      'description' => (string) $media->description,
      'pubDate' => (string) $item->{'pubDate'},
      'program' => (string) $cloudcast->program,
      'category' => (string) $media->category,
      'field_expected_filename' => $item->{'title'}
    );
  }

  return $data;
}

/**
 * Helper function to check if 'show' node exists, based on GUID field.
 */
function media_telvue_connect_importer_show_exists($guid) {
  $query = new EntityFieldQuery;
  $query->entityCondition('entity_type', 'node')
    ->propertyCondition('type', 'cm_show')
    ->fieldCondition('field_cmb_show_guid', 'value', $guid, '=');
  $result = $query->execute();

  if (isset($result['node'])) {
    $nid = array_keys($result['node']);
    $nid = $nid[0];
    return $nid;
  }
  else {
    return FALSE;
  }
}

/**
 * Helper function to check if 'show' node exists, based on field_expected_filename
 */
function media_telvue_connect_importer_advanced_show_exists($field_expected_filename) {
  $query = new EntityFieldQuery;
  $query->entityCondition('entity_type', 'node')
    ->propertyCondition('type', 'cm_show')
    ->fieldCondition('field_expected_filename', 'value', $field_expected_filename, '=');
  $result = $query->execute();

  if (isset($result['node'])) {
    $nid = array_keys($result['node']);
    $nid = $nid[0];
    return $nid;
  }
  else {
    return FALSE;
  }
}

/**
 * Helper function to create Show Node.
 */
function media_telvue_connect_importer_create_show($item) {
  if (MTCI_SYNC_MODE == 'basic') {
    // Node object
    $node = new stdClass();
    // @todo need proper source in xml
    $node->title = $item['title'];
    $node->type = 'cm_show';
    node_object_prepare($node);
    $node->language = LANGUAGE_NONE;
    $node->uid = 1;
    $node->status = 1;
    $node->promote = 0;
    $node->comment = 0;

    // Telvue connect id
    $node->field_cmb_show_guid[LANGUAGE_NONE][]['value'] = $item['link'];

    // field_show_vod
    // TODO why is this called embed code?
    $embed_code = $item['link'];

    // How does this magically work?
    $provider = media_internet_get_provider($embed_code);
    $file = $provider->save();

    // Add Fields?
    if ($file) {
      $node->field_show_vod[LANGUAGE_NONE][] = array(
        'fid' => $file->fid,
        'display' => 1,
        'description' => NULL,
      );
    }

    // Production Date/Original Air Date [ field: field_show_production_date ]
    $show_date = strtotime($item['pubDate']);
    $node->field_show_production_date[LANGUAGE_NONE][]['value'] = date("Y-m-d H:i:s", $show_date);

    // Description
    $node->field_description[LANGUAGE_NONE][]['value'] = $item['description'];

    // Tags
    // Existing tag
    if ($tag_tid = media_telvue_connect_importer_find_tag_by_title($item['category']) ) {
      $node->field_tags[LANGUAGE_NONE][]['tid'] = $tag_tid;
    }
    // Create tag and assign to node.
    else {
      $term = media_telvue_connect_importer_create_tag($item['category']);
      if ($term) {
        $node->field_tags[LANGUAGE_NONE][]['tid'] = $term->tid;
      }
    }

    // Save node
    $node = node_submit($node);

    try {
      node_save($node);
      $success = TRUE;

      // We need to do this after the node is saved to the DB, not during creation.
      // Series
      // Create record in og_membership table.

      // GID is the NID of the series.
      // We get this by matching the cloudcast:program value to a series title.
      $gid = media_telvue_connect_importer_find_series_by_title($item['program']);

      if ($gid) {
        $og_values = array(
          'entity_type' => 'node',
          'entity' => $node,
          'state' => OG_STATE_ACTIVE,
        );

        // Add this node to the group
        $og_group = og_group('node', $gid, $og_values);
      }
    }
    catch (Exception $e) {
      $success = FALSE;
    }

    return $success;
  }
}

/**
 * Helper function to update 'show' node, used by advanced.
 */
function media_telvue_connect_importer_update_show($node, $item) {
  if (MTCI_SYNC_MODE == 'advanced') {
    // Set Telvue connect guid
    $node->field_cmb_show_guid[LANGUAGE_NONE][0]['value'] = $item['link'];

    // Create media asset
    $embed_code = $item['link'];
    // How does this magically work?
    $provider = media_internet_get_provider($embed_code);
    $file = $provider->save();

    // Save show to field_show_vod
    if ($file) {
      $node->field_show_vod[LANGUAGE_NONE][0] = array(
        'fid' => $file->fid,
        'display' => 1,
        'description' => NULL,
      );
    }

    // Update/save node
    try {
      field_attach_update('node', $node);
      $success = TRUE;
    }
    catch (Exception $e) {
      $success = FALSE;
    }

    return $success;
  }
}

/**
 * Helper function to find Taxonomy tags by title
 */
function media_telvue_connect_importer_find_tag_by_title($title) {
  $tid = db_query("SELECT tid FROM {taxonomy_term_data} WHERE name = :name",
    array(
      ':name' => $title)
    )->fetchField();

  if (!empty($tid)) {
   return $tid;
  }
  else {
    return FALSE;
  }
}

/**
 * Helper function to create a Taxonomy Tag
 */
function media_telvue_connect_importer_create_tag($title) {
  $vocab = taxonomy_vocabulary_machine_name_load('tags');

  $term = (object) array(
   'name' => $title,
   //'description' => 'This is term 1',
   'vid' => $vocab->vid,
  );

  try {
    $success = taxonomy_term_save($term);
  }
  catch (Exception $e) {
    $success = FALSE;
  }

  return $success;
}

/**
 * Helper function to find show by title.
 */
function media_telvue_connect_importer_find_series_by_title($title) {
  $nid = db_query("SELECT nid FROM {node} WHERE type = 'cm_project' AND title = :title",
    array(
      ':title' => $title)
    )->fetchField();

  if (!empty($nid)) {
   return $nid;
  }
  else {
    return FALSE;
  }
}
