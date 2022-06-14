<script>
import Icons from './Icons.svelte';
import Home from './Index.svelte';
import Wifi from './Wifi.svelte';
import Button from './Button.svelte';

export let title;
const routes = [
    { title: 'Home', component: Home, path: '/',     icon: 'home'       },
    { title: 'Wifi', component: Wifi, path: '/wifi', icon: 'connection' },
];

let selectedIndex = routes.findIndex(e => e.path == window.location.pathname)
let selected = routes[selectedIndex];
let menu = false;

function changeComponent(event) {
  let i = event.srcElement.id;
  selected = routes[i];
  selectedIndex = i;
  window.history.pushState({}, 
    selected.title, 
    `${window.location.origin}${selected.path}`
  );
  menu = false;
}

function closeMenu(event) {
  menu = false;
}

function toggleMenu(event) {
  console.log('toggle', menu);
  menu = !menu;
  console.log('toggle', menu);
}
</script>

<style type="text/sass" global>
@import 'styles/global.sass'

.nav-item
  display: block
  float: left
  padding: $gutter
  margin-bottom: $gutter 
  *
    pointer-events: none
  h5
    line-height: $nav-icon-size
    vertical-align: middle
    padding-left: $gutter
  svg 
    display: block
    height: $nav-icon-size

h1, h2
  line-height: 28px
  vertical-align: middle
  float: left

.header
  margin-bottom: $gutter

#contentHeader button
  margin-right: $gutter * 2

#leftBar
  display: none

@media (max-width: 768px)
  #leftBar
    display: none
    z-index: 50
    height: 100%
    position: fixed
    top: 0px
    buttom: 0px
    left: 0px

@media (min-width: 768px)
  #leftBar
    display: block
    border-right: 0px !important

#closeButton
  position: absolute
  right: $gutter * 2
  bottom: $gutter * 2
</style>


<Icons />
<!-- Left Bar -->
<div id="leftBar" 
     class="xg2 lg2 md3 sm6"
     style={menu? 'display: block !important': ''} >

  <!-- Main Title -->
  <div class="all10 p3 header logo">
    <h1>{title}</h1>
  </div>

  <!-- App navigation -->
  <nav class="all10 p3">
    {#each routes as n, i}
      <a title={n.title} id={i} 
         href={n.path}
         class={selectedIndex==i ? 'nav-item active' : 'nav-item'} 
         on:click={changeComponent}>
        <svg class="all2"><use xlink:href={"#icon-" + n.icon}></use></svg>
        <h5 class="all8">{n.title}</h5>
      </a>
    {/each}
  </nav>
  <div class="sm7"></div>
  <Button id="closeButton" icon="arrow-left" cls="mobile sm2" 
      on:click={closeMenu} />
</div>

<!-- Content -->
<div class="xg6 lg7 md7">
  <div id="contentHeader" class="all10 p3 header">
    <Button icon="menu" cls="mobile" on:click={toggleMenu} />
    <h2>{selected.title}</h2>
  </div>
  <div id="content" class="all10 p3">
    <!-- this is where our main content is placed -->
    <svelte:component this={selected.component}/>
  </div>
</div>

