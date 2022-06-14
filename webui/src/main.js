import App from './App.svelte';

const app = new App({
	target: document.body,
	props: {
    title: "ESP8266!",
	}
});

export default app;
