/* RegionalDSKSFC — Project Website — script.js */
(function () {
    'use strict';

    /* --------------------------------------------------------
       Mobile nav toggle
    -------------------------------------------------------- */
    var toggle   = document.getElementById('nav-toggle');
    var navMenu  = document.getElementById('nav-menu');

    if (toggle && navMenu) {
        toggle.addEventListener('click', function () {
            var isOpen = navMenu.classList.toggle('is-open');
            toggle.setAttribute('aria-expanded', String(isOpen));
        });

        // Close menu when a link is clicked (mobile)
        navMenu.querySelectorAll('a').forEach(function (link) {
            link.addEventListener('click', function () {
                navMenu.classList.remove('is-open');
                toggle.setAttribute('aria-expanded', 'false');
            });
        });

        // Close menu when clicking outside
        document.addEventListener('click', function (e) {
            if (!toggle.contains(e.target) && !navMenu.contains(e.target)) {
                navMenu.classList.remove('is-open');
                toggle.setAttribute('aria-expanded', 'false');
            }
        });
    }

    /* --------------------------------------------------------
       Active nav link on scroll
    -------------------------------------------------------- */
    var sections = Array.from(document.querySelectorAll('section[id]'));
    var navLinks = Array.from(document.querySelectorAll('.nav-links a[href^="#"]'));

    function setActiveLink() {
        var scrollY = window.scrollY + 80;   // offset for sticky nav
        var currentId = '';

        sections.forEach(function (sec) {
            if (scrollY >= sec.offsetTop) {
                currentId = sec.id;
            }
        });

        navLinks.forEach(function (link) {
            var isActive = link.getAttribute('href') === '#' + currentId;
            link.classList.toggle('active', isActive);
        });
    }

    window.addEventListener('scroll', setActiveLink, { passive: true });
    setActiveLink();

    /* --------------------------------------------------------
       Last-updated date for the slides card
       To update: change the string below to "Month DD, YYYY"
    -------------------------------------------------------- */
    var lastUpdatedEl = document.getElementById('last-updated');
    if (lastUpdatedEl) {
        // Change this string when you upload a new version of slides.pdf
        var SLIDES_DATE = 'not yet uploaded';
        lastUpdatedEl.textContent = SLIDES_DATE;
    }

    /* --------------------------------------------------------
       Warn when placeholder links are clicked
    -------------------------------------------------------- */
    var meetingLink = document.getElementById('meeting-notes-link');
    if (meetingLink && meetingLink.getAttribute('href') === 'MEETING_NOTES_LINK') {
        meetingLink.addEventListener('click', function (e) {
            e.preventDefault();
            alert('Replace MEETING_NOTES_LINK in index.html with your Google Docs share URL.');
        });
    }

})();
