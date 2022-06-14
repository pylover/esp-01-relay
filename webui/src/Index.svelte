<script>
import { onMount } from 'svelte';
import Section from './Section.svelte';
import Button from './Button.svelte';

let loading = true;
let p = {};

onMount(async () => {
  const res = await fetch(`/status.json`);
  if (res.ok) {
    p = await res.json();
    p.title = `${p.zone}.${p.name}`;
    loading = false;
  }
  else {
    throw new Error(res.text());
  }
});


async function toggleBoot(event) {
  console.log('Toggle boot');
  const res = await fetch('/boots', {method: 'TOGGLE'});
}

async function reboot(event) {
  console.log('Reboot');
  const res = await fetch('/', {method: 'REBOOT'});
}

const sysinfo = [
  { title: 'Name', value: 'title'},
  { title: 'Uptime', value: 'uptime'},
  { title: 'Boot', value: 'boot'},
  { title: 'Version', value: 'version'},
  { title: 'Free Memory', value: 'free'},
  { title: 'RTC', value: 'rtc'},
];
</script>

<style type="text/sass">
@import 'styles/variables.sass'
#index > .row
  min-height: $nav-icon-size
  > div:first-child
    padding-right: $gutter * 2
    text-align: right
  > div
    height: 100% 
    line-height: $nav-icon-size
    vertical-align
</style>

<div id="index">
  <Section title="System Status" icon="stats-dots" />
  {#each sysinfo as n, i}
    <div class="all10 row">
      <div class="xg2 lg3 md4 sm5">{n.title}:</div>
      <div class="xg8 lg7 md6 sm5">{p[n.value]}</div>
    </div>
  {/each}

  <Section title="System Actions" icon="hammer" />
  <div class="all10 row">
    <Button title="Reboot" icon="switch" on:click={reboot} />
    <Button title="Toggle Boot" icon="loop2" on:click={toggleBoot} />
  </div>

  <Section title="Source Code" icon="github" />
  <p>
   Checkout these repositories to find the source code and figure out how to 
   cook it!
  </p>
  <ul>
    <li>
      <a href="https://github.com/pylover/esp8266-env">
       https://github.com/pylover/esp8266-env</a>
    </li>
    <li>
      <a href="https://github.com/pylover/esp8266-fota">
       https://github.com/pylover/esp8266-fota</a>
    </li>
  </ul>

  <Section title="Bug Report" icon="bug" />
  <p>
    Visit 
    <a href="https://github.com/pylover/esp8266-fota/issues">here</a>
    to submit any issue.
  </p>

  <Section title="Command Line Interface" icon="terminal" />
  <p>
    You may install 
    <a href="https://github.com/pylover/unspy">unspy</a>
    to control the device via CLI.
  </p>
</div>


