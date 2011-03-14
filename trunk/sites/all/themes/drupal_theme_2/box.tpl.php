<div class="box">  
<div class="art-Block clear-block block block-<?php print $block->module ?>" id="block-<?php print $block->module .'-'. $block->delta; ?>">
      <div class="art-Block-tl"></div>
      <div class="art-Block-tr"></div>
      <div class="art-Block-bl"></div>
      <div class="art-Block-br"></div>
      <div class="art-Block-tc"></div>
      <div class="art-Block-bc"></div>
      <div class="art-Block-cl"></div>
      <div class="art-Block-cr"></div>
      <div class="art-Block-cc"></div>
      <div class="art-Block-body">
      
    <?php if ($title): ?>
<div class="art-BlockHeader">
          <div class="art-header-tag-icon">
              <div class="t">
        <?php echo $title; ?>
</div>
          </div>
      </div>
    <?php endif; ?>
<div class="art-BlockContent content">
        <div class="art-BlockContent-body">
    
      <?php echo $content; ?>

        </div>
    </div>
        

      </div>
  </div>
      
</div>
