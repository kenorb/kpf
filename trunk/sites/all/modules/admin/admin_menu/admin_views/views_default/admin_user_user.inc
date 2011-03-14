<?php

$view = new view;
$view->name = 'admin_user_user';
$view->description = 'List, add, and edit users.';
$view->tag = 'admin';
$view->view_php = '';
$view->base_table = 'users';
$view->is_cacheable = FALSE;
$view->api_version = 2;
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('fields', array(
  'name' => array(
    'label' => 'Name',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'link_to_user' => 1,
    'overwrite_anonymous' => 0,
    'anonymous_text' => '',
    'exclude' => 0,
    'id' => 'name',
    'table' => 'users',
    'field' => 'name',
    'relationship' => 'none',
  ),
  'status' => array(
    'label' => 'Active',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'type' => 'yes-no',
    'not' => 0,
    'exclude' => 0,
    'id' => 'status',
    'table' => 'users',
    'field' => 'status',
    'relationship' => 'none',
  ),
  'rid' => array(
    'label' => 'Roles',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'type' => 'ul',
    'separator' => ', ',
    'empty' => '',
    'exclude' => 0,
    'id' => 'rid',
    'table' => 'users_roles',
    'field' => 'rid',
    'relationship' => 'none',
  ),
  'created' => array(
    'label' => 'Member for',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'date_format' => 'time ago',
    'custom_date_format' => '',
    'exclude' => 0,
    'id' => 'created',
    'table' => 'users',
    'field' => 'created',
    'relationship' => 'none',
  ),
  'access' => array(
    'label' => 'Last access',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'date_format' => 'time ago',
    'custom_date_format' => '',
    'exclude' => 0,
    'id' => 'access',
    'table' => 'users',
    'field' => 'access',
    'relationship' => 'none',
  ),
  'edit_node' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'text' => '',
    'exclude' => 0,
    'id' => 'edit_node',
    'table' => 'users',
    'field' => 'edit_node',
    'relationship' => 'none',
  ),
  'delete_node' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'text' => '',
    'exclude' => 0,
    'id' => 'delete_node',
    'table' => 'users',
    'field' => 'delete_node',
    'override' => array(
      'button' => 'Override',
    ),
    'relationship' => 'none',
  ),
));
$handler->override_option('sorts', array(
  'created' => array(
    'order' => 'DESC',
    'granularity' => 'second',
    'id' => 'created',
    'table' => 'users',
    'field' => 'created',
    'relationship' => 'none',
  ),
));
$handler->override_option('filters', array(
  'uid' => array(
    'operator' => 'in',
    'value' => '',
    'group' => '0',
    'exposed' => TRUE,
    'expose' => array(
      'use_operator' => 0,
      'operator' => 'uid_op',
      'identifier' => 'name',
      'label' => 'Name',
      'optional' => 1,
      'remember' => 1,
      'reduce' => 0,
    ),
    'id' => 'uid',
    'table' => 'users',
    'field' => 'uid',
    'relationship' => 'none',
  ),
  'status' => array(
    'operator' => '=',
    'value' => 'All',
    'group' => '0',
    'exposed' => TRUE,
    'expose' => array(
      'operator' => '',
      'identifier' => 'status',
      'label' => 'Active',
      'optional' => 1,
      'remember' => 1,
    ),
    'id' => 'status',
    'table' => 'users',
    'field' => 'status',
    'relationship' => 'none',
  ),
  'rid' => array(
    'operator' => 'and',
    'value' => array(),
    'group' => '0',
    'exposed' => TRUE,
    'expose' => array(
      'use_operator' => 0,
      'operator' => 'rid_op',
      'identifier' => 'rid',
      'label' => 'Roles',
      'optional' => 1,
      'single' => 0,
      'remember' => 1,
      'reduce' => 0,
    ),
    'id' => 'rid',
    'table' => 'users_roles',
    'field' => 'rid',
    'relationship' => 'none',
    'reduce_duplicates' => 0,
  ),
));
$handler->override_option('access', array(
  'type' => 'perm',
  'perm' => 'administer users',
));
$handler->override_option('cache', array(
  'type' => 'none',
));
$handler->override_option('title', 'Users');
$handler->override_option('use_ajax', TRUE);
$handler->override_option('items_per_page', 50);
$handler->override_option('use_pager', '1');
$handler->override_option('style_plugin', 'bulk');
$handler->override_option('style_options', array(
  'grouping' => '',
  'override' => 1,
  'sticky' => 1,
  'order' => 'desc',
  'columns' => array(
    'name' => 'name',
    'status' => 'status',
    'rid' => 'rid',
    'created' => 'created',
    'access' => 'access',
    'edit_node' => 'edit_node',
  ),
  'info' => array(
    'name' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'status' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'rid' => array(
      'separator' => '',
    ),
    'created' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'access' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'edit_node' => array(
      'separator' => '',
    ),
  ),
  'default' => 'created',
  'execution_type' => '2',
  'display_type' => '0',
  'hide_select_all' => 0,
  'skip_confirmation' => 0,
  'display_result' => 1,
  'merge_single_action' => 0,
  'selected_operations' => array(
    'user_user_operations_block' => 'user_user_operations_block',
    'views_bulk_operations_delete_user_action' => 'views_bulk_operations_delete_user_action',
    'views_bulk_operations_user_roles_action' => 'views_bulk_operations_user_roles_action',
    'system_send_email_action' => 'system_send_email_action',
    'user_user_operations_unblock' => 'user_user_operations_unblock',
    'user_block_ip_action' => 0,
    'user_block_user_action' => 0,
    'system_message_action' => 0,
    'views_bulk_operations_action' => 0,
    'views_bulk_operations_script_action' => 0,
    'views_bulk_operations_argument_selector_action' => 0,
    'system_goto_action' => 0,
  ),
));
$handler = $view->new_display('system', 'System', 'system_1');
$handler->override_option('path', 'admin/user/user');