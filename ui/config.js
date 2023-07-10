const env = {
  API_URL: '',
};

switch (window.location.hostname) {
  case 'localhost':
  case '127.0.0.1':
    if (window.location.port === '5000') {
      // Testing environment
      env.API_URL = 'http://localhost:5001';
    } else if (window.location.port === '4000') {
      // Staging environment
      env.API_URL = 'http://localhost:4001';
    } else if (window.location.port === '3000') {
      // Production environment
      env.API_URL = 'http://localhost:3001';
    } else {
      console.error('Port not recognized. Please check your configuration.');
    }
    break;
  case 'konigsberg':
  case 'konigsberg.disi.unitn.it':
  case '192.168.184.52':
    if (window.location.port === '5000') {
      // Testing environment
      env.API_URL = 'http://192.168.184.52:5001';
    } else if (window.location.port === '4000') {
      // Staging environment
      env.API_URL = 'http://konigsberg.disi.unitn.it:4001';
    } else if (window.location.port === '3000') {
      // Staging environment
      env.API_URL = 'http://konigsberg.disi.unitn.it:3001';
    } else {
      console.error('Port not recognized. Please check your configuration.');
    }
    break;
  case 'cricca.disi.unitn.it':
    // Production environment
    env.API_URL = '//cricca.disi.unitn.it/cyclerank-demo/api';
    break;
  default:
    console.error('Environment not recognized. Please check your configuration.');
}

window.env = env;
