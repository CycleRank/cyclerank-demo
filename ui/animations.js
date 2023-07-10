(function () {
  
  // dynamic menu selection https://codepen.io/r_e_d_ant/pen/oNWZgpy
  const menuToggle = document.querySelector('.menu-toggle');
  const bxMenu = document.querySelector('.bx-menu');
  const bxX = document.querySelector('.bx-x');
  
  const navBar = document.querySelector('.navbar');
  
  // --- open menu ---
  
  bxMenu.addEventListener('click', (e)=> {
      if(e.target.classList.contains('bx-menu')){
          navBar.classList.add('show-navbar');
          bxMenu.classList.add('hide-bx');
          bxX.classList.add('show-bx');
      }
  })
  
  // --- close menu ---
  
  bxX.addEventListener('click', (e)=> {
      if(e.target.classList.contains('bx-x')){
          navBar.classList.remove('show-navbar');
          bxMenu.classList.remove('hide-bx');
          bxX.classList.remove('show-bx');
      }
  })

    
  // animation on scroll https://codepen.io/AriMoren/pen/wjxVJd
  scrollDivRight = document.getElementById("scrollDiv");
  tryItOut = document.getElementById("try_it_out");
  main_exampleTitle = document.getElementById("main_exampleTitle");
  examplePermalinks = document.getElementById("examplePermalinks");

  comparePage = document.getElementById("output");

  var header = document.getElementById("myHeader");
  var sticky = header.offsetTop;

  var myScrollFunc = function () {
    // make header sticky, except for compare page. FIXME: if the results are put in a separate page, make a distinction between compare and results here
    if (window.pageYOffset > sticky && !comparePage) {
      header.classList.add("sticky");
    } else {
      header.classList.remove("sticky");
    }

    // index page only (animations)
    if (scrollDivRight && main_exampleTitle && examplePermalinks) {
      var y = window.scrollY;
      if (y >= 200) {
        scrollDivRight.className = "rightMenu show"
        // scrollDivRight.className = "rightMenu show goLeft"
      } else {
        scrollDivRight.className = "rightMenu hide goRight"
      }
      if (y >= 500) {
        main_exampleTitle.className = "show"
        examplePermalinks.className = "show"
      } else {
        main_exampleTitle.className = "hide goLeft2"
        examplePermalinks.className = "hide goLeft2"
      }
    }
  };

  window.addEventListener("scroll", myScrollFunc);

})()

function showBlock(firstIdToHide, secondIdToHide="") {
  var x = document.getElementById(firstIdToHide);
  if (x.style.display === "none") {
    x.style.display = "block";
  } else {
    x.style.display = "none";
  }
  if (secondIdToHide != "") {
    var y = document.getElementById(secondIdToHide);
    if (y.style.display === "none") {
      y.style.display = "block";
    } else {
      y.style.display = "none";
    }
  }
}

// theme swap: https://medium.com/@haxzie/dark-and-light-theme-switcher-using-css-variables-and-pure-javascript-zocada-dd0059d72fa2
// function to set a given theme/color-scheme
function setTheme(themeName) {
  localStorage.setItem('theme', themeName);
  document.documentElement.className = themeName;
}

// function to toggle between light and dark theme
function toggleTheme() {
  var b = document.querySelector('body');
  if (localStorage.getItem('theme') === 'theme-dark'){
      setTheme('theme-light');
      b.style.setProperty('color', 'black');
  } else {
      setTheme('theme-dark');
      b.style.removeProperty('color');
  }
}

// Immediately invoked function to set the theme on initial load
(function () {
  var b = document.querySelector('body');
  if (localStorage.getItem('theme') === 'theme-light') {
      setTheme('theme-light');
      b.style.setProperty('color', 'black');
      document.getElementById("slider").checked = true;
  } else {
      setTheme('theme-dark');
      b.style.removeProperty('color');
      document.getElementById("slider").checked = false;
  }
})();
