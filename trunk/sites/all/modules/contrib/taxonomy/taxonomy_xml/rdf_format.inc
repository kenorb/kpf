<?php
/* double-commented to avoid conflict with svn
// $Id: rdf_format.inc,v 1.1.4.12.2.25 2011/01/19 10:10:11 dman Exp $
*/

/**
 * @file
 *   Include routines for RDF parsing and taxonomy/term creation.
 * @author dman http://coders.co.nz
 *
 * 2009-09 Code to support bnodes (internal references to other nodes within an
 * RDF document) prompted by a patch contribution from by Remzi Celebi
 *
 */

define('TAXONOMY_XML_RDF_NS', 'http://www.w3.org/1999/02/22-rdf-syntax-ns#');
define('TAXONOMY_XML_TYPE', TAXONOMY_XML_RDF_NS . 'type');
define('TAXONOMY_XML_UNTYPED', 'UNTYPED');

define('TAXONOMY_XML_RDFS_NS', 'http://www.w3.org/2000/01/rdf-schema#');

// See  http://www.w3.org/2004/12/q/doc/rdf-labels.html
define('TAXONOMY_XML_CONTENTLABEL_NS', 'http://www.w3.org/2004/12/q/contentlabel#');
define('TAXONOMY_XML_CATEGORY', TAXONOMY_XML_CONTENTLABEL_NS . 'Category');

// OWL - Web Ontology Language - Formalized Meaning and Logic
define('TAXONOMY_XML_OWL_NS', 'http://www.w3.org/2002/07/owl#');

define('TAXONOMY_XML_W3C_WN', 'http://www.w3.org/2006/03/wn/wn20/');
define('TAXONOMY_XML_W3C_WN_SCHEMA', TAXONOMY_XML_W3C_WN . 'schema/');

// Dublin Core - Metadata standards
define('TAXONOMY_XML_DC_NS', 'http://purl.org/dc/elements/1.1/');
// Simple Knowledge Organization System - Structural information management
define('TAXONOMY_XML_SKOS_NS', 'http://www.w3.org/2004/02/skos/core#');
// Taxonomic Database Working Group - Biodiversity Information Standards (LSIDs etc)
define('TAXONOMY_XML_TDWG_NS', 'http://rs.tdwg.org/ontology/voc/Collection#');

// Freebase data
define('TAXONOMY_XML_FB_NS', 'http://rdf.freebase.com/ns/');

define('TAXONOMY_XML_GEO_NS', 'http://www.w3.org/2003/01/geo/wgs84_pos#');
define('TAXONOMY_XML_KML_NS', 'http://earth.google.com/kml/2.1/');

// Arc can either be in this modules folder, or in a system-shared location
define('TAXONOMY_XML_ARC1_PATH', drupal_get_path('module', 'taxonomy_xml') . '/arc');

// Made up just for myself
define('TAXONOMY_XML_DRUPAL_NS', 'http://drupal.org/project/taxonomy_xml#');


/**
 * Return information about this format
 *
 * Implimentation of (internal) taxonomy_xml_HOOK_format_info()
 */
function taxonomy_xml_rdf_format_info() {
  $formats = array(
    'RDF' => array(
      'id' => 'RDF',
      'name' => 'RDF/XML',
      'module' => 'taxonomy_xml',
      'parser_callback' => 'taxonomy_xml_rdf_parse',
    ),
  );
  return $formats;
}

/**
 * Read in RDF taxonomies and vocabularies. Create vocabs and terms as needed.
 *
 * See formats.html readme for information about the RDF input supported.
 *
 * Targets include:
 *   ICRA      Content Rating  http://www.icra.org/vocabulary/
 *   WordNet   Lexicon http: //wordnet.princeton.edu/
 *   SUMO      http://www. ontologyportal.org/
 *   Freebase
 *
 * ... and the ontologies found at http://www.schemaweb.info/ that implement
 * appropriate parts of the RDF Schema "rdfs" (eg Classes with subclassOf)
 *
 * This function takes care of the parsing of RDF syntax into attributes
 * (predicates). Actual term creation and logic is done by taxonomy_xml.module,
 * mostly in taxonomy_xml_rdf_make_term() taxonomy_xml_canonisize_predicates().
 *
 * @param $data the string containing XML/RDF
 * @param $vid int Vocab ID. May be modified by ref if this process creates a
 * new vocab to use.
 * @param $url optional source URL this RDF came from if needed to resolve GUIDs
 * etc. Cannot work for uploads.
 *
 * @return a list of resulting terms. FALSE on failure.
 */
function taxonomy_xml_rdf_parse(&$data, &$vid, $url = NULL) {
  // See if it's really a different file we need to parse
  @list($resource_url, $anchor) = split('#', $url);

  $triples = taxonomy_xml_rdf_parse_data_into_triples($data, $resource_url);

  if (empty($triples)) {
    drupal_set_message(t("No data extracted from input %url.", array('%url' => $resource_url)));
    return FALSE;
  }

  // If a specific ID was defined in the file, this means we just need to load
  // that one. This will help break things up for batches, and also allow us to
  // grab only sub-trees from big files.
  if (!empty($anchor)) {
    watchdog('taxonomy_xml', "
      We were only asked about #%anchor in this document.
      Reducing the data down to statements about that.",
      array('%anchor' => $anchor),
      WATCHDOG_DEBUG
    );
    $triples = taxonomy_xml_rdf_get_statements_about($url, $triples);

    if (empty($triples)) {
      watchdog('taxonomy_xml',
        "Found no information about  %anchor in the document !resource_url",
        array('%anchor' => $anchor, '!resource_url' => l($resource_url, $resource_url)),
        WATCHDOG_WARNING
      );
    }
  }

  // The RDF input may come in several flavours,
  // Resources of the following 'types' may be cast into taxonomy terms for our purposes.
  // That is, an rdf:Class is a Drupal:term
  //
  // These are the things to look for.
  // Add to this list as needed
  //
  $term_types = array(
    TAXONOMY_XML_RDF_NS . 'Property',
    TAXONOMY_XML_DC_NS . 'subject',
    TAXONOMY_XML_RDFS_NS . 'Class',
    TAXONOMY_XML_OWL_NS . 'Class',
    TAXONOMY_XML_W3C_WN_SCHEMA . 'Word',
    TAXONOMY_XML_W3C_WN_SCHEMA . 'NounWordSense',
    TAXONOMY_XML_W3C_WN_SCHEMA . 'NounSynset',
    TAXONOMY_XML_CONTENTLABEL_NS . 'Category',
    TAXONOMY_XML_SKOS_NS . 'Concept',
    'urn:lsid:ubio.org:classificationbank',
    'http://prismstandard.org/namespaces/2.0/pcv/Descriptor',
    TAXONOMY_XML_FB_NS . 'common.topic', // A freebase core 'topic'
      // freebase 'topic' is a superclass of useful things like 'music.genre'

    // @see http://www.alexandria.ucsb.edu/gazetteer/FeatureTypes/FTT_metadata.htm
    'http://www.esri.com/metadata/catalog/adl/#PT',
  );

  // A Drupal 'vocabulary' is represented by an owl:Ontology
  // or other similar shaped constructs
  $vocabulary_types = array(
    TAXONOMY_XML_OWL_NS . 'Ontology',
    TAXONOMY_XML_RDF_NS . 'Description',
    'http://www.w3.org/2001/12/Glossary',
    TAXONOMY_XML_TDWG_NS . 'Collection',
    TAXONOMY_XML_SKOS_NS . 'ConceptScheme',
    # Resources that are of type fb:type_profile are often collections of 'topics'
    # thus, the are analogous to our 'vocabulary'
    TAXONOMY_XML_FB_NS . 'freebase.type_profile',
    TAXONOMY_XML_FB_NS . 'base.ontologies.ontology_class',
  );

  // Group the statements about things together.
  // This will flatten the structure a little, and discards namespaces
  $resources_by_type = taxonomy_xml_convert_triples_to_sorted_objects($triples);

  // The resources are all initialized as data objects.
  // Resource types we expect to be dealing with are just vocabs and terms.
  if (! $anchor ) {
    // Message is just noise if using anchors.
    watchdog('taxonomy_xml', "
      Found %count different <strong>kinds</strong> of resources
      in the named input : %types
      ",
      array(
        '%count' => count($resources_by_type),
        '%types' => join(', ', array_keys($resources_by_type))
      ),
      WATCHDOG_INFO
    );
  }
  if (count($resources_by_type) == 0) {
    watchdog('taxonomy_xml', "
      It sure doesn't look like this is any useful sort of RDF source.
      Probably need to do content-negotiation or something. Aborting.",
      array('%url' => ''),
      WATCHDOG_WARNING
    );
    return;
  }

  #dpm($resources_by_type);
  $vocab_uri = NULL;
  if ($vid == 0) {
    // We've been asked to use the vocab described in the source file.
    // If the vid has already been set, we ignore vocab definitions found in the file

    // Scan the sorted objects for vocabulary definitions
    // Hopefully there's only one vocab per file, but loop anyway
    $vocabularies = array();
    foreach ($vocabulary_types as $vocabulary_type) {
      if (isset($resources_by_type[$vocabulary_type]) && is_array($resources_by_type[$vocabulary_type])) {
        foreach ($resources_by_type[$vocabulary_type] as $vocab_uri => &$vocabulary_handle) {
          $vocabularies[$vocab_uri] = &$vocabulary_handle;
        }
      }
    }
    drupal_set_message(t("Found %count resources to be used as vocabulary definitions", array('%count' => count($vocabularies))));

    if (! $vocabularies) {
      // Create a placeholder.
      $vocabularies[] = (object) array('name' => 'Imported Vocabulary');
    }
    $vid = taxonomy_xml_absorb_vocabulary_definitions($vocabularies);
    // $vocabularies now contains a keyed array of target vocabularies the terms may be put into
    // $vid is the default one (most common is one vocab per input file) to be used unless otherwise defined per-term.

    if (empty($vid)) {
      drupal_set_message(t("No vocabulary to add terms to, aborting."), 'error');
      return FALSE;
    }
  }
  else {
    // Else using a form-selected vocob.
    $vocabularies[$vid] = taxonomy_vocabulary_load($vid);
    // Note that a pre-made vocab already in the system will not have predicates
    // any more. Don't count on them
  }
  foreach ($vocabularies as $vocabulary) {
    module_invoke_all('taxonomy_xml_vocabulary_presave', $vocabulary);
  }

  //
  // VOCAB set up, start on TERMS...
  ///

  #dpm(array('vocabs are' => $vocabularies));

  // Gather the resources that will become terms.
  // Slightly long way (not using array_merge), as I need to merge indexed and by reference
  $terms = array();
  foreach ($term_types as $term_type) {
    // watchdog('taxonomy_xml', 'Adding all %term_type to the list of terms to be processed', array('%term_type' => $term_type), WATCHDOG_DEBUG);
    if (isset($resources_by_type[$term_type]) && is_array($resources_by_type[$term_type])) {
      foreach ($resources_by_type[$term_type] as $guid => &$term_handle) {
        // Grab name/label early for debugging and indexing
        $predicates = @$term_handle->predicates;
        if (isset($predicates['label'])) {
          $term_handle->name = reset($predicates['label']);
        }
        $terms[$guid] = &$term_handle;
      }
    }
  }

  // A FB import MAY also tell us a vocabulary is a top-level term
  // FB allows it to be both. We don't, it breaks things
  if (isset($terms[$vocab_uri])) {
    watchdog('taxonomy_xml', 'Vocab %vocab_uri was allegedly both a vocab and a term. Drupal can not handle that. Simplifing', array('%vocab_uri' => $vocab_uri), WATCHDOG_NOTICE);
    unset($terms[$vocab_uri]);
  }

  // Some of the RDF documents I've been fed DO NOT DEFINE A TYPE for their primary subject.
  // Neither
  // http://www.ubio.org/authority/metadata.php nor
  // http://biocol.org/ nor
  // http://lsid.tdwg.org/
  // return RDF that says WHAT the data is. Those that use LSIDs have a type encoded in the Identifier itself :-/

  // I end up with a collection of data but no idea what it's really talking about.
  // But IF an entity is rdf:about="THIS URL" then we will take a leap and assume that is our target lump of data.
  // ... this worked for biocol input
  foreach ( (array) @$resources_by_type[TAXONOMY_XML_UNTYPED] as $identifier => $untyped_lump) {
    if ($identifier == $url) {
      // Looks like this was the specific thing we were looking for
      $terms[$identifier] = $untyped_lump;
    }
  }

  // FREEBASE only
  // Special case for freebase.
  // If we are reading a top-level topic type page
  // eg http://www.freebase.com/tools/explore/music/genre
  // type = fb:type_profile
  // then it may contain a list of 'instances' which represent our desired
  // member terms.
  $fb_vocab_type = TAXONOMY_XML_FB_NS . 'freebase.type_profile';

  foreach ((array) @$resources_by_type[$fb_vocab_type] as $vocab_uri => $vocabulary) {
    $instances = @$vocabulary->predicates['type.type.instance'];
    if (! empty($instances) ) {
      // I've got a list of URIs that represent terms, but not even a name for them
      // The system will still hopefully be able to work it out from just that.
      watchdog('taxonomy_xml', "
        FREEBASE: Each <em>instance</em> listed in a freebase <em>type profile</em>
        will be imported as a term.",
        array(), WATCHDOG_INFO
      );
      foreach ($instances as $term_guid) {
        $terms[$term_guid] = $placeholder_term = (object) array(
          'guid' => $term_guid,
          'vid' => $vid,
        );
        // Queue a full lookup of this item
        taxonomy_xml_add_term_to_batch_queue($placeholder_term);
        watchdog('taxonomy_xml',
          "Queuing a full retrieval of term !term_uri it for later retrieval and import",
          array(
            '!term_uri' => l($term_guid, $term_guid),
          )
          , WATCHDOG_INFO
        );
      } // loop over all term 'instances' mentioned by the vocab
    }

    // Extra diagnostic - freebase-specific
    if (isset($vocabulary->predicates)) {
      $instance_count = $vocabulary->predicates['freebase.type_profile.instance_count'];
      if ($instance_count > count($instances) ) {
        watchdog('taxonomy_xml', "
          FREEBASE: The topic set definition claims there are %instance_count
          topic instances in the set, but I can see only %actual_count.
          Some data may be missing from this doc that I am unable to retrieve.
          ",
          array(
            '%instance_count' => reset($instance_count),
            '%actual_count' => count($instances),
          ), WATCHDOG_WARNING
        );
      }
    }

    // Resources that are being processed as vocabs are NOT also terms.
    // But the freenet schema labels topic sets as 'topics' themselves.
    // Unset this so as not to make a vocab definition a member of itself.
    unset($resources_by_type[TAXONOMY_XML_FB_NS . 'common.topic'][$vocab_uri]);
  }

  if (! $anchor ) {
    // Shh.
    drupal_set_message(t("Found %count resources to be imported as terms into vocabulary %vid", array('%count' => count($terms), '%vid' => $vid)));
  }

  //
  // START MAKING TERMS
  //
  foreach ($terms as $identifier => &$term) {
    #drupal_set_message(t("Reviewing term %identifier '%name' and analyzing its properties", array('%identifier' => $identifier, '%name' => $term->name)));

    $term->identifier = $identifier;
    if (!isset($term->vid)) {
      // This is just a default fallback. Imported terms should really have already chosen their vid.
      $term->vid = $vid;
    }
    taxonomy_xml_rdf_make_term($term);
  }

  // Now the terms are all happily created, create their relationships
  // Couldn't do so until they had all been given tids.
  taxonomy_xml_set_term_relations($terms);
  // Note this will not yet affect terms that have been queued for later processing.
  // Such terms will need to attach themselves to the parent terms themselves.

  #dpm(array('After re-linking, we now have all terms set' => $terms));

  foreach ($vocabularies as $vocabulary) {
    module_invoke_all('taxonomy_xml_vocabulary_postsave', $vocabulary);
  }

  return $terms;
}

/**
 * Invoke the ARC parser on the given data.
 *
 * Uses some minor caching if the base $url is the same.
 * If the requested base URI is the same as the previous one, you'll get a
 * cached version, but those data objects are not held onto in a true cache
 * array.
 * This will be optimal for one big file being called all the time (an all-in-
 * one taxonomy), and NOT fill up with crud if lots of different files are
 * requested once (as happens when spidering).
 */
function taxonomy_xml_rdf_parse_data_into_triples($data, $url) {
  static $old_triples, $old_url;
  if (!empty($url) && $url == $old_url) {
    // re-using parser cache
    return $old_triples;
  }

  watchdog('taxonomy_xml', "Parsing RDF from !url", array('!url' => l($url, $url)), WATCHDOG_INFO);

  // Use ARC parser
  require_once TAXONOMY_XML_ARC1_PATH . "/ARC_rdfxml_parser.php";

  $parser_args = array(
    "bnode_prefix" => "genid",
    "base" => "",
  );
  $parser = new ARC_rdfxml_parser($parser_args);
  $triples = $parser->parse_data($data);
  if (! is_array($triples)) {
    drupal_set_message(t("Problem parsing input %message", array('%message' => $triples)), 'error');
    return;
  }
  watchdog('taxonomy_xml', "
    %count data triples (atomic statements) found in the source RDF doc",
    array('%count' => count($triples)),
    WATCHDOG_INFO
  );
  # dpm($triples);
  # drupal_set_message('<pre>' . print_r($triples, 1) . '</pre>');

  // Caching
  $old_url = $url;
  $old_triples = $triples;
  return $triples;
}


/**
 * Filter a big list of triples down to only the ones about one subject;
 */
function taxonomy_xml_rdf_get_statements_about($guid, $triples) {
  $filtered_statements = array();
  foreach ($triples as $triplenum => $statement) {
    @$subject_uri = $statement['s']['uri'];
    switch ($statement['s']['type']) {
      case 'uri':
        $subject_uri = $statement['s']['uri'];
        break;
      case 'bnode':
        $subject_uri = trim($statement['s']['bnode_id']);
        break;
      default:
        $subject_uri = trim($statement['s']['val']);
    }
    if ($subject_uri == $guid) {
      $filtered_statements[$triplenum] = $statement;
    }
    // else ignore
  }
  return $filtered_statements;
}


/**
 * Create the placeholder and fill in the values for this term - NOT its
 * relationships yet.
 */
function taxonomy_xml_rdf_make_term(&$term) {
  $identifier = $term->identifier;
  # drupal_set_message(t("Reviewing term %identifier '%name' and analyzing its properties", array('%identifier' => $identifier, '%name' => @$term->name)));

  // When running in batch, children will have a hard time finding their
  // parents if they only know them by source-localized ID (probably a URI)
  // and the destination-taxonomy (here) HASN'T REMEMBERED THAT INFO.
  // Because taxonomy.module just doesn't.
  // We require some other module (taxonomy_enhancer is good) to save that
  // metadata for us so the child can find its target later.
  // This is our 'identifier' - the REMOTE identifier not the local one.
  if (!isset($term->guid)) {
    $term->guid = $identifier;
  }

  // Build term from data

  // Convert all input predicates into attributes on the object
  // the taxonomy.module will understand
  taxonomy_xml_canonicize_predicates($term);

  // Ensure name is valid
  if (empty($term->name)) {

    // which of these approaches is correct?

    // Look, if we don't even have a name, creating a term is a waste of time.
    // RDF feeds commonly consist of a bunch of pointers, we can't invent placeholders until we know a little more.
    // Let's not do this.
    #drupal_set_message(t("Not enough information yet (not even a name) to create a term referred to as %identifier. Not creating it yet.", array('%identifier' => $identifier)));
    #unset($terms[$identifier]);
    #continue;

    // If the parent is trying to link to a child thats not yet made,
    // we probably don't know a proper name or label.

    // Fallback to a name, identifier derived (roughly) from the URI identifier - not always meaningful, but all we have in some contexts.
    $term->name = taxonomy_xml_label_from_uri($identifier);
    watchdog('taxonomy_xml', "
      We were unable to find a specific label for the term
      referred to as %identifier.
      Guessing that %name will be good enough.",
      array('%identifier' => $identifier, '%name' => $term->name),
      WATCHDOG_NOTICE
    );

    // Still, this causes problems if queuing data about terms that are not yet loaded
    // - such as those that are ONLY referenced by URI with no human name (Freenet)
    // Our munged names are temporary until the full data is retrieved.

    if (empty($term->name)) {
      // Still not set?
      // This should be impossible - all subjects must have a URI
      // But who knows what wierdness the input gave us
      drupal_set_message(t("
        A term called %identifier didn't produce any readable name to use. "
        , array('%identifier' => $identifier)), 'error');
      continue;
    }
  }

  $force_new = variable_get('taxonomy_xml_duplicate', FALSE);
  // See if a definition matching this terms name already exists in the DB.
  // Build on that.
  $existing_term = taxonomy_xml_get_term_by_guid($term->guid, $term->vid);
  if (! $existing_term) {
    $existing_term = _taxonomy_xml_get_term_placeholder($term->name, $term->vid, $force_new);
  }

  #dpm(array('old term' => $existing_term, 'new term' => $term));
  // Merge the old term objects properties into this one. Really just want its tid, but there may be more info I should not lose.
  // New input takes precedence over older data. Old data just fills in the gaps.
  foreach ((array) $existing_term as $key => $value) {
    if (! isset($term->$key)) {
      $term->$key = $value;
    }
  }
  // The term object is now as tidy as it can be as a self-contained entity.
  # dpm($term);

  if (variable_get('taxonomy_xml_reuseids', FALSE)) {
    // TODO this has not been tested since migration from D5!
    // MAINTAIN IDS
    // Because this is likely to be used with a site-cloning set-up,
    // it would help if we tried to match IDs
    // OTOH, doing so could be very messy for other situations.
    // So,
    //  iff there is no pre-existing term with this id,
    //  create this one as a clone with the old ID.
    // This requires a little DB sneakiness.
    if (isset($term->internal_id) && ! taxonomy_term_load($term->internal_id)) {
      $term->tid = $term->internal_id;
      drupal_set_message(t("Doing sneaky import of %term_name re-using the internal id = %term_id", array('%term_name' => $term->name, '%term_id' => $term->internal_id)));
      db_query("INSERT INTO {term_data} (tid, name, description, vid, weight) VALUES (%d, '%s', '%s', %d, %d)", $term->tid, $term->name, $term->description, $term->vid, $term->weight);

      # sequences is gone in D6. Will inserting beyond the auto-increment self-correct?
      $current_id = db_last_insert_id('term_data', 'tid');
      if ($current_id < $term->tid) {
        // This is probably now MYSQL specific.
        db_query("ALTER TABLE {term_data} AUTO_INCREMENT = %d;", $term->tid);
      }
    }
  }

  # Here's where last-minute data storage done by other modules gets set up
  // module_invoke_all doesn't do pass-by-reference, so do our own loop.
  foreach (module_implements('taxonomy_xml_term_presave') as $module) {
    $function = $module . '_' . 'taxonomy_xml_term_presave';
    $function($term);
  }

  ////////////////////////////
  // Assist taxonomy_enhancer
  if (module_exists('taxonomy_enhancer')) {
    $fields = taxonomy_enhancer_get_fields_by_vocabulary($term->vid);
    foreach ($fields as  $te_field) {
      if (isset($term->predicates[$te_field->title])) {
        // Looks like a predicate of the same name as a te field exists. Set it
        foreach ($term->predicates[$te_field->title] as $delta => $value) {
          $term->fields[$te_field->fid][$delta] = array(
            'value' => $term->predicates[$te_field->title][$delta],
            'format' => 0,
          );
        }
      }
    }
  }
  #dpm($term);
  // finished taxonomy_enhancer (should be delegated to a helper hook)
  /////////////////////////


  #dpm(array("ready to save" => $term));
  $save_term = (array) $term;
  $status = taxonomy_save_term($save_term);

  # Need to ensure the new hook callbacks fire also during that term saving

  // Re-retrieve the new term definition,
  // just in case anything extra happened to it during processing
  $new_term = taxonomy_xml_get_term_by_name_from_vocab($term->name, $term->vid);
  if (! $new_term) {
    drupal_set_message(t("
      It seems like we failed to create and retrieve a term called %term_name",
      array('%term_name' => $term->name)),
      'error'
    );
  }
  // Merge retrieved values back over our main definition so the handles are up-to-date
  foreach ((array) $new_term as $key => $value) {
    $term->$key = $value;
  }

  if ( $status == SAVED_NEW ) {
    // Just remember this is fresh - for useful feedback messages.
    $term->taxonomy_xml_new_term = TRUE;
  }

  // It's possible that not all the referenced items were available in the current document/loop
  // Add referred items to the import queue for later processing
  taxonomy_xml_add_all_children_to_queue($term);
  $term->taxonomy_xml_presaved = TRUE; // A flag to avoid double-processing

  // Allow other hooks to do last-minute processing
  // http://drupal.org/node/791376
  foreach (module_implements('taxonomy_xml_term_postsave') as $module) {
    $function = $module . '_' . 'taxonomy_xml_term_postsave';
    $function($term);
  }
  
  return $term;
  // end term-construction;
}

/**
 * Compile triple statements into information objects again.
 *
 * Returns a nested array, Indexed on their URI/id, and grouped by type
 * (references so we can change them).
 *
 * Not all RDF data objects declare exactly what they are, some just announce
 * that they exist.
 * Some guesswork is done if their identifier is an LSID - we can deduce
 * what type of object it refers to. An explicit RDF:type will take priority
 * over this assumption.
 */
function taxonomy_xml_convert_triples_to_sorted_objects(&$triples) {
  // Triples are boringly granular bits of information.
  // Merge them.
  $resources = array();
  $resources_by_type = array();
  foreach ($triples as $triplenum => $statement) {

    @$subject_uri = $statement['s']['uri'];

    // Inspect the subject type and get value of the specified field
    // if type is 'uri' get the value of 'uri' or type is 'bnode' then get 'bnode_id'
    // by Remzi Celebi
    switch ($statement['s']['type']) {
      case 'uri':
        $subject_uri = $statement['s']['uri'];
        break;
      case 'bnode':
        $subject_uri = trim($statement['s']['bnode_id']);
        break;
      default:
        $subject_uri = trim($statement['s']['val']);
    }

    if (! isset($resources[$subject_uri]) ) {
      // Create placeholder if this is the first occurance of this subject
      $resources[$subject_uri] = (object) array();
    }
    $subject = &$resources[$subject_uri];

    # dpm(array("Processing a statement about $subject_uri" => $statement));

    // Namespaces are boring, Simplify the predicates
    // TODO - revisit if namespaces are needed
    $predicate = taxonomy_xml_rdf_shortname($statement['p']);

    // All predicates are stored in arrays, setup placeholder
    if (! isset($subject->predicates[$predicate]) ) {
      $subject->predicates[$predicate] = array();
    }

    // Set the object of this subject, into its predicate array.
    // The object may be a literal, an identifier, or a bnode
    // In the case of an identifier or bnode, that is a reference to
    // something found elsewhere.

    // Find and apply the $object_val
    switch ($statement['o']['type']) {
      case 'uri':
        $object_uri = $object_val = $statement['o']['uri'];

        // Also make a placeholder for the object, for convenience.
        // It's not much fun referring to something that doesn't exist.
        if (! isset($resources[$object_uri]) ) {
          $resources[$object_uri] = (object) array();
        }

        // Only add uniques, Keeps clutter down
        if (! in_array($object_val, $subject->predicates[$predicate])) {
          $subject->predicates[$predicate][] = $object_val;
        }
        break;

      case 'literal':
        @$object_val = trim($statement['o']['val']);
        // If there appear to be alternative versions of the same thing,
        // save both, keyed by language if appropriate.
        if ($lang = $statement['o']['lang']) {
          /**
           * Need to do extra strangeness to support multiple values (synonyms)
           * x multiple languages!
           *
           * <skos:altLabel xml:lang="en">Emergency relief</skos:altLabel>
           * <skos:altLabel xml:lang="en">Emergency assistance in disasters</skos:altLabel>
           * <skos:altLabel xml:lang="en">Disaster assistance</skos:altLabel>
           * <skos:altLabel xml:lang="fr">Panic</skos:altLabel>
           *
           * becomes
           * $term->predicates['altLabel'] = array(
           *   'en' => 'Emergency relief',
           *   'en:1' => 'Emergency assistance in disasters',
           *   'en:2' => 'Disaster assistance',
           *   'fr' => 'Panic',
           * )
           *
           * The actual key is not expected to be re-used at the moment, it's
           * just informational. I haven't thought this through.
           *
           */
          $key = $lang;
          if (isset($subject->predicates[$predicate][$lang])) {
            $key = $lang . ":" . count($subject->predicates[$predicate]);
          }
          $subject->predicates[$predicate][$key] = $object_val;
        }
        else {
          $subject->predicates[$predicate][] = $object_val;
        }
        break;

      case 'bnode':
        $object_val = $statement['o']['bnode_id'];
        if (isset($statement['o']['bnode_id'] )) {
          $subject->predicates[$predicate][$statement['o']['bnode_id']] = $object_val;

          // 2010-05-27 dman
          // Generally we discard bnodes after sucking the usefulness out of them
          // BUT if they are really useful structured data, hang on to them for
          // possible later, deeper processing.
          $subject->bnodes[$predicate][$statement['o']['bnode_id']] = &$resources[$object_val];
        }
        break;
    }

    if ($predicate == 'type') {
      // Very important info!
      $subject->type = $object_val;
      // Sort it! (by reference)
      $resources_by_type[$subject->type][$subject_uri] = &$subject;
      // It's legal for a resource to have more than one 'type' (see Freenet)
      // This is fine, a pointer to the item is placed in both bags.
    }
    if ($predicate == TAXONOMY_XML_NAME) {
      $subject->name = $object_val;
    }

    // This is very memory-intensive for big vocabs. Try to clean up:(
    unset($triples[$triplenum]);
  }

  // A special work-around for irregular data.
  // Scan the full array for any lost (untyped) data,
  // Make some guesses if we can, and collect the rest into a catch-all 'untyped' list.
  $unknown_resources = array();
  foreach ($resources as $guid => &$subject) {

    // While we are looping,
    // Make a guess at its original, internal ID
    // grabbing the last numeric bit from the id in the document
    // eg from '#vocab/1' or '#vocabulary:1' or #term33
    // Be very generic and forgiving in the format we look for
    $parts = preg_split('|[^\d]|', $guid);
    $last_num = array_pop($parts);
    if (is_numeric($last_num)) {
      $subject->internal_id = $last_num;
    }
    // Not really used much yet.

    // Anyway, check the type. If not known,
    // This could confuse us later, make a note for analysis.
    if (! isset($subject->type)) {

      $url_parts = @parse_url($guid);
      if (isset($url_parts['host'])) {
        // looks (roughly) like a valid URI - No need to complain about legal external references.
        // It's only unresolvable ones that could be a problem.
        continue;
      }

      // If the identifier of this resource is an 'LSID'
      // then the type is sort of embedded in the string as the 'namespace'.
      // See if we can extract it.
      if ($lsid = taxonomy_xml_parse_lsid($guid)) {
        $resources_by_type[$lsid['type']][$guid] = &$subject;
        continue;
      }

      // Nope, it's a total UFO, make a note for debugging
      if (drupal_substr($guid, 0, 2) != '_:') {
        // Ignore 'Here' nodes produced by ARC, eg '_:genid1', '_:genid2'
        $unknown_resources[$guid] = &$subject;
      }
    }

  }
  if ($unknown_resources) {
    // Just FYI, make a note about the quality of data found.
    // Do not complain about URLs - this is quite normal.

    watchdog('taxonomy_xml', "
      Found %count Unsorted (untyped) resources.
      They are entities that are the subject of a statement,
      but I don't know what <em>type</em> of thing they are.
      Not sure what I'll do with these.
      They are things that have had statements made about them ..
      that I don't recognise.
      Probably just extra data found in the input and ignored.
      ID was: %unknown",
      array(
        '%count' => count($unknown_resources),
        '%unknown' => join(', ', array_keys($unknown_resources))
      ),
      WATCHDOG_DEBUG
    );
    $resources_by_type[TAXONOMY_XML_UNTYPED] = $unknown_resources;
  }

  return $resources_by_type;
}

/**
 * Choose a string from an array of language-tagged possibilities
 *
 * Util func to help read complex RDF statements.
 */
function taxonomy_xml_get_literal_string($values) {
  if (! is_array($values)) {
    return trim($values);
  }
  // May need to choose language
  if (count($values) == 1) {
    $out = array_pop($values);
  }
  else {
    // TODO add language selector
    if ($label = @$values['en']) {
      $out = $label;
    }
    else {
      // fine, whatever
      $out = array_pop($values);
    }
  }
  return trim($out);
}

/**
 * Return the shorthand label of a potentially long RDF URI
 *
 * EG, for http://www.w3.org/1999/02/22-rdf-syntax-ns#Property
 * return 'Property'
 * ... for sanity
 *
 * Also flatten LSIDs - which are used like URIs but just are NOT as useful
 *
 */
function taxonomy_xml_rdf_shortname($uri) {

  // For LSID simplification, flatten assorted RDF-LSID-Predicates (from any authority) into their simple name
  if (($lsid = taxonomy_xml_parse_lsid($uri)) && ($lsid['namespace'] == 'predicates') ) {
    return $lsid['identifier'];
  }

  // If I recognised namespaces, I could use short ones. That would be fine.
  // But I don't want to start conflicting with rdf.modules ones.
  #if (function_exists('rdf_uri_to_qname')) {
  #  return rdf_uri_to_qname($uri);
  #}
  # yeah, by trimming namespaces and making guesses, now can't put them back in.
  # Needs revision.

  $parts = parse_url($uri);
  $shortname = !empty($parts['fragment']) ? $parts['fragment'] : (
    !empty($parts['query']) ? $parts['query'] : (
      basename($parts['path'])
      ));
  // The proper method for guessing simple names is probably documented elsewhere.
  // ... this does the trick for now.
  return $shortname;
}

/**
 * Return an XML/RDF document representing this vocab
 *
 * I'd like to use ARC libraries, but it doesn't appear to include an RDF
 * serializer output method, only an input parser...
 *
 * Uses PHP DOM to create DOM document and nodes.
 *
 * We use namespaces carefully here, although it may create wordy output if the
 * DOM is not optimizing the declarations for us. Still, best to be explicit, it
 * would seem.
 *
 * The URI used to refer to other resources is based on the source document
 * location, eg
 * http://this.server/taxonomy_xml/{vid}/rdf#{tid}
 *
 * Preamble should look something like:
 *
 * <rdf:RDF xmlns:rdf ="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
 *   xmlns: rdfs="http://www.w3.org/2000/01/rdf-schema#"
 *   xmlns: owl="http://www.w3.org/2002/07/owl#"
 *
 */
function taxonomy_xml_rdf_create($vid, $parent = 0, $depth = -1, $max_depth = NULL) {
  $vocabulary = taxonomy_vocabulary_load($vid);

  $domcontainer = taxonomy_xml_rdf_document();
  $dom = $domcontainer->ownerDocument;
  #dpm(array(domcontainer => $domcontainer, dom => $dom));

  // If this was a cannonic vocab, we would use a full URI as identifiers
  // however, we make an ID up instead.
  $vocabulary->machine_name = 'vocabulary-' . $vocabulary->vid;

  // If it is a features vocabulary, its cannonic ID is overloaded in the 'module' field.
  // Makes enough sense. Use that
  if (strpos($vocabulary->module, 'features_') === 0) {
    // Simply display the existing machine name if we have one.
    $vocabulary->machine_name = substr($vocabulary->module, 9);
  }
  if (empty($vocabulary->guid)) {
    $vocabulary->guid = url('taxonomy_xml/vocabulary-' . $vocabulary->vid , array('absolute' => TRUE) );
  }

  // Define the vocab
  taxonomy_xml_add_vocab_as_rdf($domcontainer, $vocabulary);

  // and more details?

  // Now start adding terms.
  // They are listed as siblings, not children of the ontology
  $tree = module_invoke('taxonomy', 'get_tree', $vid, $parent, $depth, $max_depth);
  $tree = taxonomy_get_tree($vid, $parent, $depth, $max_depth);
  taxonomy_xml_add_terms_as_rdf($domcontainer, $tree, $vocabulary);

  // Clean up the namespace hack?
  $domcontainer->removeattributens(TAXONOMY_XML_RDFS_NS, 'rdfs:title');
  $domcontainer->removeattributens(TAXONOMY_XML_OWL_NS, 'owl:hack');

  $result = $dom->savexml();

  // Minor layout tweak for readability
  $result = preg_replace('|(<[^<]*/[^>]*>)|', "$1\n", $result);
  $result = preg_replace('|><|', ">\n<", $result);
  # dpm($result);
  return $result;
}

/**
 * Set up an RDF document preamble.
 * Returns a document, also sets the passed handle to the RDF node that content
 * should land in
 *
 */
function taxonomy_xml_rdf_document() {
  $dom = new domdocument('1.0', 'UTF-8');

  $dom->appendchild($dom->createcomment(xmlentities("
    This file was created by Drupal taxonomy_xml import/export tool.
    http://drupal.org/project/taxonomy_xml
    /* // \$Id: rdf_format.inc,v 1.1.4.12.2.25 2011/01/19 10:10:11 dman Exp $ */

    The RDF schema in this file is intended to follow the Working Draft
    described at http://www.w3.org/TR/wordnet-rdf/ for the notation of
    thesauri and taxonomies.
    ")
  ));
  $dom->appendchild($dom->createprocessinginstruction('xml-stylesheet', 'href="render-taxonomy-rdf.xsl" type="text/xsl"' ));

  $domcontainer = $dom->createelementns(TAXONOMY_XML_RDF_NS, 'rdf:RDF');
  #  $rdfnode->setattribute('xmlns', TAXONOMY_XML_RDFS_NS);
  $dom->appendchild($domcontainer);

  taxonomy_xml_rdf_add_namespace($domcontainer, TAXONOMY_XML_RDFS_NS, 'rdfs');
  taxonomy_xml_rdf_add_namespace($domcontainer, TAXONOMY_XML_OWL_NS, 'owl');

  // Invoke additional module support hooks may need to add namespaces.
  $hook = 'taxonomy_xml_rdf_document_setup';
  foreach (module_implements($hook) as $module) {
    $function = $module .'_'. $hook;
    $function($domcontainer);
  }

  return $domcontainer;
}

/** 
 * Adding namespaces is fiddly.
 */
function taxonomy_xml_rdf_add_namespace($element, $uri, $prefix) {
  // By appending a namespaced att, the extra namespaces appear at the top.
  // Then remove them again, but don;t remove the namespace!
  // Otherwise the appear everywhere. There must be a better way
  $element->setattributens($uri, "{$prefix}:hack", "Initializing namespace in PHP is hard" );
  $element->removeattribute("{$prefix}:hack");
}

/**
 * Create a vocabulary definition (just the def, not its terms) and insert it
 * into the given document element.
 *
 * @param $domcontainer an XML dom document, modified by ref.
 * @param $vocabulary a vocab object
 */
function taxonomy_xml_add_vocab_as_rdf(&$domcontainer, $vocabulary) {
  $dom = $domcontainer->ownerDocument;

  // Describe the vocabulary itself
  $vocabnode = $dom->createelementns(TAXONOMY_XML_OWL_NS, 'owl:Ontology');
  $domcontainer->appendchild($vocabnode);

  $vocabnode->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:ID', $vocabulary->machine_name);
  #$vocabnode->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:about', url('taxonomy_xml/' . $vocabulary->vid . '/rdf', array('absolute' => TRUE) ) );
  // Apparently rdf:ID and rdf:about on the same element is illegal. Hm.
  // Use xml:base instead. http://www.ibm.com/developerworks/xml/library/x-tiprdfai.html
  $domcontainer->setattribute('xml:base', $vocabulary->guid );

  $vocabnode->appendchild(
    $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:label', xmlentities($vocabulary->name))
  );

  if ($vocabulary->description) {
    $vocabnode->appendchild(
      $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:comment', xmlentities($vocabulary->description))
    );
  }

  $vocabnode->appendchild(
    $dom->createelementns(TAXONOMY_XML_OWL_NS, 'owl:versionInfo', xmlentities(format_date($_SERVER['REQUEST_TIME'], 'long')))
  );
}

/**
 * Given a list of terms, append definitions of them to the passed DOM container
 *
 * Following w3c, SUMO and Wordnet examples (tho not any explicit instructions,
 * taxonomy terms are modelled as rdfs:Class objects structured using rdfs:
 * subClassOf statements.
 *
 * Sample from Wordnet:
 *
 * <Class rdf:about="http://xmlns.com/wordnet/1.6/Cat">
 *   <label>Cat  [ 1 ]</label>
 *   <comment>feline mammal usually having thick soft fur and being unable
 * to roar; domestic cats; wildcats</comment>
 *   <subClassOf>
 *     <Class rdf:about="http://xmlns.com/wordnet/1.6/Feline" />
 *   </subClassOf>
 * </Class>
 *
 * I'm copying that syntax.
 *
 * @param $termlist a FLAT array of all terms, internally cross-referenced to
 * each other defining the tree stucture
 */
function taxonomy_xml_add_terms_as_rdf(&$domcontainer, $termlist, $vocabulary) {
  if (! $termlist) {
    return;
  }
  $dom = $domcontainer->ownerDocument;

  // Allow submission of a single term
  if (! is_array($termlist)) {
    $termlist = array($termlist);
  }

  foreach ($termlist as $term) {
    module_invoke_all('taxonomy_term_load', $term);
    #dpm($term);
    $termnode = $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:Class');
    $termnode->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:ID', 'term-' . $term->tid );
    $domcontainer->appendchild($termnode);

    if ($guid = taxonomy_xml_get_term_guid($term)) {
      $termnode->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:about', $guid );
    }
dpm(xmlentities($term->name));
    $termnode->appendchild(
      $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:label', xmlentities($term->name))
    );

    if ($term->description) {
      $termnode->appendchild(
        $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:comment', xmlentities($term->description) )
      );
    }

    $vocab_ref = $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:isDefinedBy');
    $vocab_ref->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:resource', '#' . $vocabulary->machine_name );
    $termnode->appendchild($vocab_ref);

    foreach ((array) taxonomy_get_related($term->tid) as $relatedid => $relatedterm) {
      $related_node = $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:seeAlso', xmlentities($relatedterm->name) );
      $related_node->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:resource', '#term-' . $relatedid );
      $termnode->appendchild($related_node);
    }

    $synonyms = taxonomy_get_synonyms($term->tid);
    // TODO - figure out the right syntax for synonym
    // I'm using 'equivalentClass' ... although that's really intended for merging different vocabs.
    foreach ((array) $synonyms as $synonymname) {
      $synonymnode = $parent_node = $dom->createelementns(TAXONOMY_XML_OWL_NS, 'owl:equivalentClass', xmlentities($synonymname) );
      $termnode->appendchild($synonymnode);
    }

    if (!empty($term->parents)) {
      foreach ((array) $term->parents as $parentid) {
        $parentlist = array();
        if ($parentid) {
          $parentlist[$parentid] = $parent =  taxonomy_get_term($parentid);
          $parent_node = $dom->createelementns(TAXONOMY_XML_RDFS_NS, 'rdfs:subClassOf');
          $parent_node->setattributens(TAXONOMY_XML_RDF_NS, 'rdf:resource', '#term-' . $parentid );
          $parent_node->setattributens(TAXONOMY_XML_RDFS_NS, 'rdfs:title', xmlentities($parent->name) );
          $termnode->appendchild($parent_node);
        }
      }
    }

    // Additional module support
    // eg taxonomy_image, geotaxonomy, path
    //
    $hook = 'taxonomy_xml_rdf_export_term';
    // Can't use module_invoke as we need pass-by-ref
    foreach (module_implements($hook) as $module) {
      $function = $module .'_'. $hook;
      $function($termnode, $term);
    }

    # dpm(array('adding term to rdf' => $term));
    #$termnode->appendchild($dom->createcomment(print_r($term, 1)));

    // workaround for large vocabs - extend runtime indefinately
    set_time_limit(10);
  }
  // Done all terms in list
}


/**
 * Check ARC RDF library is available
 *
 * (should look into starting to use chaostools to manage plugins?)
 *
 * For now, if it returns anything but an empty string, that means there's
 * a fail.
 */
function taxonomy_xml_rdf_requirements() {
  $requirements = array();
  if (! is_readable(TAXONOMY_XML_ARC1_PATH . "/ARC_rdfxml_parser.php")) {
    $requirements['taxonomy_xml_rdf'] = array(
      'value' => t('ARC1 RDF Parser is unavailable.'),
      'severity' => 1, // REQUIREMENT_WARNING,
      'description' => t('
        See <a href="!install">INSTALL.txt</a>
        for the extra features that the external
        <a href="!arc">ARC library</a> can add
        if you download it to %path.
      ', array(
          '!arc' => 'http://arc.semsol.org/',
          '!install' => url( drupal_get_path('module', 'taxonomy_xml') . '/INSTALL.txt'),
          '%path' => TAXONOMY_XML_ARC1_PATH,
        )
      ),
    );
  }
  return $requirements;
}

/**
 * Return a term as RDF. Header and all
 */
function taxonomy_xml_rdf_export_term($term, $depth = -1, $max_depth = NULL) {
  if (is_numeric($term)) {
    $term = taxonomy_get_term($term);
  }

  // Load in all extra data
  module_invoke_all('taxonomy_term_load', $term);

  $domcontainer = taxonomy_xml_rdf_document();
  $dom = $domcontainer->ownerDocument;

  taxonomy_xml_add_terms_as_rdf($domcontainer, $term);

  // Now start adding terms.
  // They are listed as siblings, not children of the ontology
  $tree = module_invoke('taxonomy', 'get_tree', $term->vid, $term->tid, $depth, $max_depth);

  taxonomy_xml_add_terms_as_rdf($domcontainer, $tree);
  $result = $dom->savexml();

  // Minor layout tweak for readability
  $result = preg_replace('|(<[^<]*/[^>]*>)|', "$1\n", $result);
  $result = preg_replace('|><|', ">\n<", $result);
  # dpm($result);
  print $result;
  exit;
}
