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

	<?php if ($block->subject): ?>
<div class="art-BlockHeader">
		    <div class="art-header-tag-icon">
		        <div class="t">	
			<h2 class="subject"><?php echo $block->subject; ?></h2>
</div>
		    </div>
		</div>    
	<?php endif; ?>
<div class="art-BlockContent content">
	    <div class="art-BlockContent-body">
	
		<?php echo $block->content; ?>

	    </div>
	</div>
	

    </div>
</div>
