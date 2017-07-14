
<div id=csdta_display style="border-style:solid;border-width:10px;
    border-color:#FF9;padding:5px;
    display:none;position:fixed;top:10px;left:10px;z-index:9999;
    background:#FFC"></div>

<script type="text/javascript">

/**
 * @fileoverview Javascript to display the UI for the debugging
 *  option "csdebug=1".  Displays a window showing the nested clearsilver
 *  templates at the part of the page that the mouse is pointing to.
 */

/**
 * @define {number} Just a symbol for inlining value Node.COMMENT_NODE.
 */
var COMMENT_NODE = 8;

window.csdebug = window.csdebug || {};

/**
 * Subnamespace we put everything here under.  "dta" stands for DHTML template
 * annotator.
 * @type {Object}
 */
csdebug.dta = csdebug.dta || {};

/**
 * One-time initialization to activate this script on a page.
 * This must be called anytime after the first time the annotation framework
 * HTML is rendered on the page.
 */
csdebug.dta.init = function() {
  /**
   * HTML classname dynamic info display element.
   * @type {string}
   */
  var CLASS_DISPLAY_ELEMENT = 'csdta_display';

  // This element is always output in in the stream surrounding this script;
  // but we are not guaranteed that it gets properly put in the DOM for
  // us to find, since the expansion could occur in some partial HTML
  // context.  That is why we check that we are able to locate this element.
  var displayContainer = document.getElementById(CLASS_DISPLAY_ELEMENT);
  if (displayContainer) {
    /**
     * HTML comment regex of open-file mark annotation.
     * @type {RegExp}
     */
    var OPEN_FILE_REGEX = /^csdta_of @/;

    /**
     * HTML comment regex of close-file mark annotation.
     * @type {RegExp}
     */
    var CLOSE_FILE_REGEX = /^csdta_cf @/;

    /**
     * HTML tagname of element containing info tree item subvalues.
     * @type {string}
     */
    var TAGNAME_INFO_WRAPPER = 'DIV';

    /**
     * Given a server filepath of a template, computes source name text
     * and source URL values to be used in displaying template source info.
     * @param {Object} result Values are returned as property value
     *     'sourcename' is assigned to the passed object.
     * @private
     */
    function computeSourceInfo_(result) {
      var filename = result.file;
      if (filename[0] == '/') {
        filename = filename.substring(1);
      }
      result.sourcename = filename;
    }

    /**
     * Cross-browser utility to get the text contained by an element.
     * @param {Element} element The subject DOM element.
     * @return {string} Contained text value.
     * @private
     */
    function getElementText_(element) {
      return element.innerText || element.textContent;
    }

    /**
     * See usage in lastConsequentialDescendant_().
     * The encoding allows you to compute key membership using the "in"
     * operator.
     */
    var OPTIONALLY_CLOSED_TAGS = {
      COLGROUP: 1,
      DD: 1,
      DT: 1,
      LI: 1,
      OPTION: 1,
      P: 1,
      TBODY: 1,
      TD: 1,
      TFOOT: 1,
      TH: 1,
      THEAD: 1,
      TR: 1
    };

    /**
     * This is an internal helper utility for doing an upward depth first
     * DOM traversal to find an annotation element for the enclosing
     * template entity context.
     * This finds last child descending down the last child branch of
     * the hierarchy starting at the passed element, skipping some
     * descents that are not consequential to finding a potential
     * location of a pertinant wrapping annotation.
     * @param {Element} element The starting DOM element.
     * @return {Element} Result descendant element or passed-in element
     *     if there isn't an appropriate descendant.
     */
    function lastConsequentialDescendant_(element) {
      while (element.hasChildNodes()) {
        // This checks against HTML element types with optional close tags.
        // When elements of these types are coded without close tags, the
        // element can "swallow" a subsequent annotation element that
        // actually pertains to a later unrelated context.
        if (!(element.tagName in OPTIONALLY_CLOSED_TAGS)) {
          break;
        }
        element = element.lastChild;
      }
      return element;
    }

    /**
     * Finds the identifying name of the annotation of the tightest
     * containing template entity context that the passed-in element
     * is in.
     * @param {Element} element The subject DOM element.
     * @return {Object?} Annotation object of the enclosing template with the
     *    following properties:
     *      1) name - The template id name.
     *      2) file - The template file name.
     *      3) element - The HTML comment element of the annotation.
     *      4) sourcename - The source name text to be used in displaying
     *           template source info.
     *    If the annotation element is not found, return null.
     * @private
     */
    function findEnclosingAnnotation_(element) {
      var nestingCount = 1;
      while (nestingCount > 0 && element) {
        // We make the pragmatic assumption that a template expansion
        // entity is a whole HTML element, since that's a natural way
        // to code reusable templates and how most templates we encounter
        // are.  With this assumption you'd expect that when we added
        // our annotation elements before and after the expansion of
        // a template entity that we would be able to find the "open"
        // annotation element simply as either a previous sibling or a
        // previous sibling of an ancestor element.  However, it breaks down
        // sometimes because some HTML elements are implicitly closed
        // without close tags: when a previous sibling is coded depending
        // on such an implicit close, then it turns out our added "open"
        // annotation element can become swallowed by it and become a child
        // (or a descendant, in general).  So what is called for is to
        // sometimes do a traversal in HTML source order, not just in our
        // DOM hierarchy containment context.  The following does an
        // upward depth first traversal in outline to cover this, with
        // lastConsequentialDescendant_() implementing shortcuts to stay
        // within our DOM containment context when possible.
        element = (element.previousSibling ?
                   lastConsequentialDescendant_(element.previousSibling) :
                   element.parentNode);
        if (element && element.nodeType == COMMENT_NODE) {
          var lastComment = element.nodeValue;
          if (lastComment.match(OPEN_FILE_REGEX)) {
            --nestingCount;
          } else if (lastComment.match(CLOSE_FILE_REGEX)) {
            ++nestingCount;
          }
        }
      }
      if (element && element.nodeType == COMMENT_NODE && nestingCount == 0) {
        // A comment with our code symbol and data value is like
        // <!--csdta_of(NAME, FILE)-->.  This step parses out the NAME
        // and FILE between ( ).
        var nodeVal = element.nodeValue;
        var idx = nodeVal.indexOf('@');
        var matched = nodeVal.substring(idx + 1).split(',');
        if (matched) {
          var annotation = {};
          annotation.name = matched[0];
          annotation.file = matched[1];
          annotation.element = element;
          computeSourceInfo_(annotation);
          return annotation;
        }
      }
      return null;
    }

    /**
     * Finds the enclosing template annotation chain of the passed-in element.
     * @param {Element} element The subject DOM element.
     * @return {Array} Array of enclosing annotation objects of the given
     *          element.
     * @private
     */
    function computeAnnotationChain_(element) {
      var annotations = [];
      while (element) {
        var annotation = findEnclosingAnnotation_(element);
        if (annotation) {
          annotations.push(annotation);
          element = annotation.element;
        } else {
          element = null;
        }
      }
      return annotations;
    }

    /**
     * Sets the info display element to show the information of the
     * annotations, and sets it to visible.
     * @param {Array} annotations Array of annotation objects containing the
     *    name and file of the enclosing templates.
     * @private
     */
    function displayAnnotations_(annotations) {
      var outputLines = [];

      var annotationLength = annotations.length;
      for (var i = 0; i < annotationLength; i++) {
        var annotation = annotations[i];
        var includeText = '';
        if (annotation.name) {
          includeText = ['>', annotation.name, ': '].join('');
        }
       outputLines.push([
          '<div style="font-size:12px; margin-bottom:0.3em">',
          includeText,
          '<span style="color:#23899e">', annotation.sourcename,
          '</span>',
          '</div>'
        ].join(''));
      }

      outputLines.reverse();  // Put containers above containees.
      if (outputLines.length > 0) {
        outputLines.push(
            '<div><small>Use a modifier key to freeze.</small></div>');
        displayContainer.innerHTML = outputLines.join('');
        displayContainer.style.display = 'block';
      }
    }

    /**
     * Cross-browser utiliy to get the source element of an event.
     * @param {Event} e The subject event.
     * @return {Element?} Associated source element or null on error.
     * @private
     */
    function getSourceElement_(e) {
      return e.target || e.srcElement;
    }

    // State used to track dragging of display element
    /**
     * This is a the dragged element (which currently will always be the
     * single display element) or null if nothing being dragged.
     * @type {Element?}
     * @private
     */
    var draggedElement_ = null;

    /**
     * x coordinate relative to left on dragged element that we're grasping
     * it at.
     * @type {number}
     * @private
     */
    var dragOffsetX_ = 0;

    /**
     * y coordinate relative to top on dragged element that we're grasping
     * it at.
     * @type {number}
     * @private
     */
    var dragOffsetY_ = 0;

    /**
     * Mouse-over handler for annotation displaying.
     * @param {Event} e The mouseover event.
     * @return {boolean} Returns true to always have the underlying page
     *     do normal event handling.
     * @private
     */
    function handleMouseoverForAnnotation_(e) {
      e = e || window.event;
      if (!draggedElement_ && !e.shiftKey && !e.altKey && !e.ctrlKey) {
        var element = getSourceElement_(e);
        if (element) {
          var annotations = computeAnnotationChain_(element);
          if (annotations) {
            displayAnnotations_(annotations);
          }
        }
      }
      return true;
    }

    /**
     * Mousedown handler that allows user to drag the info display box around.
     * @param {Event} e The mousedown event.
     * @return {boolean} Standard event handler return signal.
     * @private
     */
    function handleMousedownForDragging_(e) {
      e = e || window.event;
      var srcElem = getSourceElement_(e);
      if (srcElem == displayContainer) {
        // Initialize state for what we're dragging and its position.
        draggedElement_ = srcElem;
        dragOffsetX_ = e.clientX - parseInt(srcElem.style.left, 10);
        dragOffsetY_ = e.clientY - parseInt(srcElem.style.top, 10);

        // For IE
        draggedElement_.setCapture && draggedElement_.setCapture(true);
        return false;
      }
      return true;
    }

    /**
     * Mousemove handler supporting the draggability of the display box.
     * @param {Event} e The mousemove event.
     * @return {boolean} Standard event handler return signal.
     * @private
     */
    function handleMousemoveForDragging_(e) {
      e = e || window.event;
      if (draggedElement_) {
        // Modify state for what we're dragging.
        draggedElement_.style.left = e.clientX - dragOffsetX_ + 'px';
        draggedElement_.style.top = e.clientY - dragOffsetY_ + 'px';

        e.cancelBubble = true;
        e.stopPropagation && e.stopPropagation();
        return false;
      }
      return true;
    }

    /**
     * Mouseup handler supporting the draggability of the
     * display box.
     * @param {Event} e The mouseup event.
     * @return {boolean} Standard event handler return signal.
     * @private
     */
    function handleMouseupForDragging_(e) {
      e = e || window.event;
      var result = true;
      if (draggedElement_) {
        handleMousemoveForDragging_(e);
        // For IE
        draggedElement_.releaseCapture && draggedElement_.releaseCapture();
        result = false;
      }
      draggedElement_ = null;
      return result;
    }


    // Install the mouseover handler that displays information for any
    // element on the page.
    if (window.addEventListener) {
      document.documentElement.addEventListener(
          'mouseover',
          handleMouseoverForAnnotation_,
          false);
    } else {
      document.documentElement.attachEvent(
          'onmouseover',
          handleMouseoverForAnnotation_);
    }

    // Install event handling for info display container.
    displayContainer.onmousedown =
        handleMousedownForDragging_;
    if (window.addEventListener) {
      document.documentElement.addEventListener(
          'mousemove',
          handleMousemoveForDragging_,
          true);
      document.documentElement.addEventListener(
          'mouseup',
          handleMouseupForDragging_,
          true);
    } else {
      document.documentElement.attachEvent(
          'onmousemove',
          handleMousemoveForDragging_);
      document.documentElement.attachEvent(
          'onmouseup',
          handleMouseupForDragging_);
    }

    /**
     * Determines whether or not the above definitions executed already.
     * @type {boolean}
     * @private
     */
    csdebug.dta.constructed_ = true;
  }
};

csdebug.dta.init();

</script>
