<script>
import { createEventDispatcher } from 'svelte';
export let title = '';
export let icon;
export let cls = '';
export let id = '';
export let style = '';
const dispatch = createEventDispatcher();

function click() {
	dispatch('click', {
		text: 'Hello!'
	});
}

</script>
<style type="text/sass" global>
@import 'styles/global.sass'

button
  float: left
  padding: 6px
  margin-right: $gutter
  height: 32px
  line-height: 18px
  vertical-align: middle
  svg
    display: block
    float: left
    margin-top: -1px
    width: 18px
    height: 18px
  span
    margin-left: $gutter

</style> 
<button class={cls} {id} {style} on:click={click}>
  <svg><use xlink:href={"#icon-" + icon}></use></svg>
  {#if title}
    <span>{title}</span>
  {/if}
</button>
