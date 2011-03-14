<?php
/**
 * @file
 * Template to display each hours row.
 */
 ?>
<div class="oh-display">
  <?php print $dayname; ?> :
  <?php if ($closed) : ?>
    <span class="oh-closed"><?php print t('Closed'); ?></span>
  <?php else: ?>
    <span class="oh-display-hours"><?php print $starthours1; ?> - <?php print $endhours1; ?></span>
    <?php if ($additional) : ?>
      <span class="oh-display-hours">, <?php print $starthours2; ?> - <?php print $endhours2; ?></span>
    <?php endif; ?>
  <?php endif; ?>
</div>


