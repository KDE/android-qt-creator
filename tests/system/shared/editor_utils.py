import re;

# places the cursor inside the given editor into the given line
# (leading and trailing whitespaces are ignored!)
# and goes to the end of the line
# line can be a regex - but if so, remember to set isRegex to True
# the function returns True if this went fine, False on error
def placeCursorToLine(editor,line,isRegex=False):
    cursor = editor.textCursor()
    oldPosition = 0
    cursor.setPosition(oldPosition)
    editor.setTextCursor(cursor)
    found = False
    if isRegex:
        regex = re.compile(line)
    editorRealName = objectMap.realName(editor)
    while not found:
        currentLine = str(lineUnderCursor(editor)).strip()
        if isRegex:
            if regex.match(currentLine):
                found = True
            else:
                type(editor, "<Down>")
                if oldPosition==editor.textCursor().position():
                    break
                oldPosition = editor.textCursor().position()
        else:
            if currentLine==line:
                found = True
            else:
                type(editorRealName, "<Down>")
                if oldPosition==editor.textCursor().position():
                    break
                oldPosition = editor.textCursor().position()
    if not found:
        test.fatal("Couldn't find line matching\n\n%s\n\nLeaving test..." % line)
        return False
    cursor=editor.textCursor()
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.MoveAnchor)
    editor.setTextCursor(cursor)
    return True

# this function simply opens the context menu inside the given editor
# at the same position where the text cursor is located at
def openContextMenuOnTextCursorPosition(editor):
    rect = editor.cursorRect(editor.textCursor())
    openContextMenu(editor, rect.x+rect.width/2, rect.y+rect.height/2, 0)

# this function marks/selects the text inside the given editor from position
# startPosition to endPosition (both inclusive)
def markText(editor, startPosition, endPosition):
    cursor = editor.textCursor()
    cursor.setPosition(startPosition)
    cursor.movePosition(QTextCursor.StartOfLine)
    editor.setTextCursor(cursor)
    cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, endPosition-startPosition)
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
    cursor.setPosition(endPosition, QTextCursor.KeepAnchor)
    editor.setTextCursor(cursor)

# works for all standard editors
def replaceEditorContent(editor, newcontent):
    type(editor, "<Ctrl+A>")
    type(editor, "<Delete>")
    type(editor, newcontent)

def typeLines(editor, lines):
    if isinstance(lines, (str, unicode)):
        lines = [lines]
    if isinstance(lines, (list, tuple)):
        for line in lines:
            type(editor, line)
            type(editor, "<Enter>")
    else:
        test.warning("Illegal parameter passed to typeLines()")

# function to verify hoverings on e.g. code inside of the given editor
# param editor the editor object
# param lines a list/tuple of regex that indicates which lines should be verified
# param additionalKeyPresses an array holding the additional typings to do (special chars for cursor movement)
#       to get to the location (inside line) where to trigger the hovering (must be the same for all lines)
# param expectedTypes list/tuple holding the type of the (tool)tips that should occur (for each line)
# param expectedValues list/tuple of dict or list/tuple of strings regarding the types that have been used
#       if it's a dict it indicates a property value pair, if it's a string it is type specific (e.g. color value for ColorTip)
# param alternativeValues same as expectedValues, but here you can submit alternatives - this is for example
#       necessary if you do not add the correct documentation (from where the tip gets its content)
def verifyHoveringOnEditor(editor, lines, additionalKeyPresses, expectedTypes, expectedValues, alternativeValues=None):
    counter = 0
    for line in lines:
        expectedVals = expectedValues[counter]
        expectedType = expectedTypes[counter]
        altVal = None
        if isinstance(alternativeValues, (list, tuple)):
            altVal = alternativeValues[counter]
        counter += 1
        placeCursorToLine(editor, line, True)
        for ty in additionalKeyPresses:
            type(editor, ty)
        rect = editor.cursorRect(editor.textCursor())
        sendEvent("QMouseEvent", editor, QEvent.MouseMove, rect.x+rect.width/2, rect.y+rect.height/2, Qt.NoButton, 0)
        try:
            tip = waitForObject("{type='%s' visible='1'}" % expectedType)
        except:
            tip = None
        if tip == None:
            test.warning("Could not get %s for line containing pattern '%s'" % (expectedType,line))
        else:
            if expectedType == "ColorTip":
                __handleColorTips__(tip, expectedVals)
            elif expectedType == "TextTip":
                __handleTextTips__(tip, expectedVals, altVal)
            elif expectedType == "WidgetTip":
                test.warning("Sorry - WidgetTip checks aren't implemented yet.")
            sendEvent("QMouseEvent", editor, QEvent.MouseMove, 0, -50, Qt.NoButton, 0)
            waitFor("isNull(tip)")

# helper function that handles verification of TextTip hoverings
# param textTip the TextTip object
# param expectedVals a dict holding property value pairs that must match
def __handleTextTips__(textTip, expectedVals, alternativeVals):
    props = object.properties(textTip)
    expFail = altFail = False
    eResult = verifyProperties(props, expectedVals)
    for val in eResult.itervalues():
        if not val:
            expFail = True
            break
    if expFail and alternativeVals != None:
        aResult = verifyProperties(props, alternativeVals)
    else:
        altFail = True
        aResult = None
    if not expFail:
        test.passes("TextTip verified")
    else:
        for key,val in eResult.iteritems():
            if val == False:
                if aResult and aResult.get(key):
                    test.passes("Property '%s' does not match expected, but alternative value" % key)
                else:
                    aVal = None
                    if alternativeVals:
                        aVal = alternativeVals.get(key, None)
                    if aVal:
                        test.fail("Property '%s' does not match - expected '%s' or '%s', got '%s'" % (key, expectedVals.get(key), aVal, props.get(key)))
                    else:
                        test.fail("Property '%s' does not match - expected '%s', got '%s" % (key, expectedVals.get(key), props.get(key)))
            else:
                test.fail("Property '%s' could not be found inside properties" % key)

# helper function that handles verification of ColorTip hoverings
# param colTip the ColorTip object
# param expectedColor a single string holding the color the ColorTip should have
# Attention: because of being a non-standard Qt object it's not possible to
# verify colors which are (semi-)transparent!
def __handleColorTips__(colTip, expectedColor):
    cmp = QColor()
    cmp.setNamedColor(expectedColor)
    if cmp.alpha() != 255:
        test.warning("Cannot handle transparent colors - cancelling this verification")
        return
    dPM = QPixmap.grabWidget(colTip, 1, 1, colTip.width-2, colTip.height-2)
    img = dPM.toImage()
    rgb = img.pixel(1, 1)
    rgb = QColor(rgb)
    if rgb.rgba() == cmp.rgba():
        test.passes("ColorTip verified")
    else:
        test.fail("ColorTip does not match - expected color '%s' got '%s'" % (rgb.rgb(), cmp.rgb()))

# function that checks whether all expected properties (including their values)
# match the given properties
# param properties a dict holding the properties to check
# param expectedProps a dict holding the key value pairs that must be found inside properties
# this function returns a dict holding the keys of the expectedProps - the value of each key
# is a boolean that indicates whether this key could have been found inside properties and
# the values matched or None if the key could not be found
def verifyProperties(properties, expectedProps):
    if not isinstance(properties, dict) or not isinstance(expectedProps, dict):
        test.warning("Wrong usage - both parameter must be of type dict")
        return {}
    result = {}
    for key,val in expectedProps.iteritems():
        foundVal = properties.get(key, None)
        if foundVal != None:
            result[key] = val == foundVal
        else:
            result[key] = None
    return result
