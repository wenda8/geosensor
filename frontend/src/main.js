import { createApp } from 'vue';
import router from './router';
import App from './App.vue';


// Font Awesome
import { library } from '@fortawesome/fontawesome-svg-core';
import { fas } from '@fortawesome/free-solid-svg-icons';
import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome';

library.add(fas);

const app = createApp(App);

app.component('font-awesome-icon', FontAwesomeIcon);

app.use(router);

app.mount('#app');