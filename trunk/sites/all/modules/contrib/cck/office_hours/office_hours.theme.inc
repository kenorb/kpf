<?php


/**
 * Theme function for 'default' text field formatter.
 */
function theme_office_hours_formatter_default($element) {   
  $output = '';
  $items = array();
  $first = variable_get('date_first_day',0);
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $weekdays = date_week_days(TRUE);
  
  foreach (element_children($element) as $key => $elemkey) {
    $item = $element[$elemkey]['#item'];
    $day = check_plain($element[$elemkey]['#item']['day']);
    if (!empty($day)) {
      $endhrs = _office_hours_mil_to_tf(check_plain($item['endhours']));
      $strhrs = _office_hours_mil_to_tf(check_plain($item['starthours']));
      if ($field['hoursformat']) {
        $endhrs = _office_hours_convert_to_ampm($endhrs);
        $strhrs = _office_hours_convert_to_ampm($strhrs);
      }
      $items[$day][] = array('strhrs' => $strhrs,'endhrs' => $endhrs) ;//we're aggregating hours for days together. 
    }
  }

  foreach ($weekdays as $key => $day) {
    if (!array_key_exists($key,$items)) {
        $items[$key][]= array('closed'=>'closed'); //silly, but we need this as an array because we can't use a string offset later
    }
  }
  ksort($items);
  $items = date_week_days_ordered($items);
  $weekdays = date_week_days_ordered($weekdays);
  
  foreach ($items as $day => $hours) {
    $dayname = $weekdays[$day];
    if (!($hours[0]['closed'] && !$field['showclosed'])) { // don't output unnecessary fields
      $output .= theme('office_hours_display',$dayname, $hours);
    }
  }
  return $output;     
}

/**
 * Theme function for 'grouped' text field formatter.
 */
function theme_office_hours_formatter_grouped($element) {
  $output = '';
  $groups = array();
  $first = variable_get('date_first_day',0);
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $show_closed = $field['showclosed'];

  // get ordered weekdays
  // @todo should abbr be an option?
  $weekdays = date_week_days_abbr(TRUE);

  // create hours groups from items
  foreach (element_children($element) as $key => $elemkey) {
    $item = $element[$elemkey]['#item'];
    $day = $element[$elemkey]['#item']['day'];
    if (isset($day)) {
      $day_num = check_plain($day);
      if (is_numeric($day_num)) {
        $day_num = (int) $day_num;
        $day_name = $weekdays[$day_num];

        $endhrs = _office_hours_mil_to_tf(check_plain($item['endhours']));
        $strhrs = _office_hours_mil_to_tf(check_plain($item['starthours']));
        if ($field['hoursformat']) {
          $endhrs = _office_hours_convert_to_ampm($endhrs);
          $strhrs = _office_hours_convert_to_ampm($strhrs);
        }

        // aggregate by hours
        $group_id = "$strhrs::$endhrs";
        if (!isset($groups[$group_id])) {
          $groups[$group_id]['hours'] = array(array('strhrs' => $strhrs, 'endhrs' => $endhrs));
        }
        $groups[$group_id]['days'][$day_name] = $day_num;
        $group_map[$day_name][$group_id] = $group_id;
      }
    }
  }

  // Add grouped closed days
  foreach ($weekdays as $day_num => $day_name) {
    if (!isset($group_map[$day_name])) {
      $group_id = 'closed';
      if (!isset($groups[$group_id])) {
        $groups[$group_id]['hours'] = array(array('closed' => 'closed'));
      }
      $groups[$group_id]['days'][$day_name] = $day_num;
      $group_map[$day_name][$group_id] = $group_id;
    }
  }

  // Process groups
  foreach ($groups as $group_id => &$group) {
    // set combined days label
    _office_hours_set_group_label($group);
  }
  unset($group);


  // adjust translation weekday array for first day of week
  $weekdays = date_week_days_ordered($weekdays);

  // build output
  $output = '';
  $days_rendered = array();

  // output with ordered weekdays to render groups in order
  foreach ($weekdays as $day_num => $day_name) {
    if (!isset($days_rendered[$day_name]) && isset($group_map[$day_name])) {
      foreach ($group_map[$day_name] as $group_id) {
        if ($show_closed || $group_id != 'closed') {
          $group = $groups[$group_id];
          $output .= theme('office_hours_display', $group['label'], $group['hours']);
          // update rendered array
          foreach ($group['days'] as $g_day_name => $orig_day_num) {
            $days_rendered[$g_day_name] = TRUE;
          }
        }
      }
    }
  }

  return $output;
}

function template_preprocess_office_hours_display(&$vars) {
  $vars['starthours1'] = $vars['hours'][0]['strhrs'];
  $vars['endhours1'] = $vars['hours'][0]['endhrs'];
  $vars['starthours2'] = $vars['hours'][1]['strhrs'];
  $vars['endhours2'] = $vars['hours'][1]['endhrs'];
  $vars['additional'] = isset($vars['hours'][1]) ? TRUE : FALSE;
  $vars['closed'] = $vars['hours'][0]['closed'];
}

function theme_office_hours($element) { 
  return $element['#children'];
}

/**
 * Theme function for the office hours selector element.
 */
function theme_office_hours_select($element) {
  return theme('form_element', $element, $element['#children']);
}

/**
 * copied from content module's theme_content_multiple_values- we're taking out the draggable feature.
 *
 * Theme the hour table form
 *
 */
function theme_office_hours_multiple_values($element) {
  $field_name = $element['#field_name'];
  $field = content_fields($field_name);
  $output = '';

  if ($field['multiple'] >= 1) {
    $table_id = $element['#field_name'] .'_values';
    $order_class = $element['#field_name'] .'-delta-order';
    $required = !empty($element['#required']) ? '<span class="form-required" title="'. t('This field is required.') .'">*</span>' : '';

    $header = array(
      array(
        'data' => t('!title: !required', array('!title' => $element['#title'], '!required' => $required)),
        'colspan' => 1
      ),
    );
    $rows = array();

    // Sort items according to '_weight' (needed when the form comes back after
    // preview or failed validation)
    $items = array();
    foreach (element_children($element) as $key) {
      if ($key !== $element['#field_name'] .'_add_more') {
        $items[] = &$element[$key];
      }
    }
    
    $items = _office_hours_arrange_day($items); //this calls the function that arranges the first day of the week.

    // Add the items as table rows.
    foreach ($items as $key => $item) {
      $delta_element = drupal_render($item['_weight']);
      if (!($key % 2)) { //this is an even row, start a new row array_keys
        $cells = array (
          drupal_render($item),
        );
      }
      else { //this is an odd row
        $cells[] = drupal_render($item); //we've add the second cell;
        $rows[] = array(
        'data' => $cells,
        );
      }
    }

    $output .= theme('table', $header, $rows, array('id' => $table_id, 'class' => 'office_hours_table content-multiple-table'));
    $output .= $element['#description'] ? '<div class="description">'. $element['#description'] .'</div>' : '';
  //  $output .= drupal_render($element[$element['#field_name'] .'_add_more']);

    //drupal_add_tabledrag($table_id, 'order', 'sibling', $order_class);
  }
  else {
    foreach (element_children($element) as $key) {
      $output .= drupal_render($element[$key]);
    }
  }

  return $output;
}

/**
 * Helper function to set combined days label for hour group
 */
function _office_hours_set_group_label(&$group) {
  // reference days
  $group_days = &$group['days'];
  asort($group_days);

  // initialize label
  list($day_name, $prev_day_num) = each($group_days);
  $label_start = $day_name;
  $label_end = '';
  $label = $label_start;

  // build label
  while (list($day_name, $day_num) = each($group_days)) {
    $diff = $day_num - $prev_day_num;
    if ($diff == 1) {
      $label_end = $day_name;
    }
    else {
      if ($label_end) {
        $label .= ' - ' . $label_end;
      }

      $label_start = $day_name;
      $label_end = '';
      $label .= ', ' . $label_start;
    }

    $prev_day_num = $day_num;
  }

  // update label if all were consecutive
  if ($label_end) {
    $label .= ' - ' . $label_end;
  }

  // update group
  $group['label'] = $label;

  // clean up and return
  unset($group_days);
  return $label;
}
