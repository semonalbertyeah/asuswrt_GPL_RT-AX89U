/* Copyright (c): 2002-2005 (Germany): United Internet, 1&1, GMX, Schlund+Partner, Alturo */
function QxMenuButton(vText,vIcon,vCommand,vMenu){QxWidget.call(this);this.setHeight("auto");this.setLeft(0);this.setRight(0);this.setMinHeight(20);this.setTimerCreate(false);if(isValidString(vText)){this.setText(vText);};if(isValid(vIcon)){this.setIcon(vIcon);};if(isValid(vCommand)){this.setCommand(vCommand);};if(isValid(vMenu)){this.setMenu(vMenu);};this.addEventListener("mousedown",this._g1);};QxMenuButton.extend(QxWidget,"QxMenuButton");QxMenuButton.addProperty({name:"text",type:String});QxMenuButton.addProperty({name:"icon",type:String});QxMenuButton.addProperty({name:"menu",type:Object});proto._f1=null;proto._f2=null;proto._f3=null;proto._f4=null;proto._c1=false;proto._c2=false;proto._c3=false;proto._c4=false;proto._valueShortcut="";proto._modifyElement=function(_b1,_b2,_b3,_b4){if(_b1){if(this._c1&&!this._f1){this._e3Icon();};if(this._c2&&!this._f2){this._e3Text();};if(this._c3&&!this._f3){this._e3Shortcut();};if(this._c4&&!this._f4){this._e3Arrow();};};return QxWidget.prototype._modifyElement.call(this,_b1,_b2,_b3,_b4);};proto._modifyEnabled=function(_b1,_b2,_b3,_b4){if(this._f2){this._f2.setEnabled(_b1,_b4);};if(this._f1){this._f1.setEnabled(_b1,_b4);};return QxWidget.prototype._modifyEnabled.call(this,_b1,_b2,_b3,_b4);};proto._modifyIcon=function(_b1,_b2,_b3,_b4){this._c1=isValid(_b1);return true;};proto._modifyText=function(_b1,_b2,_b3,_b4){this._c2=isValid(_b1);return true;};proto._modifyCommand=function(_b1,_b2,_b3,_b4){if(isValid(_b1)){this._c3=true;this._valueShortcut=_b1.toString();}else {this._c3=false;this._valueShortcut="";};return true;};proto._modifyMenu=function(_b1,_b2,_b3,_b4){this._c4=isValid(_b1);return true;};proto.hasMenu=function(){return Boolean(this.getMenu());};proto._e3Icon=function(){var i=this._f1=new QxImage();i.setSource(this.getIcon());i.setAnonymous(true);i.setEnabled(this.isEnabled());i.setParent(this);i._addCssClassName("QxMenuButtonIcon");};proto._e3Text=function(){var t=this._f2=new QxContainer();t.setHtml(this.getText());t.setAnonymous(true);t.setEnabled(this.isEnabled());t.setParent(this);t._addCssClassName("QxMenuButtonText");};proto._e3Shortcut=function(){var s=this._f3=new QxContainer();s.setHtml(this._valueShortcut);s.setAnonymous(true);s.setEnabled(this.isEnabled());s.setParent(this);s._addCssClassName("QxMenuButtonShortcut");};proto._e3Arrow=function(){var a=this._f4=new QxImage();a.setSource("widgets/arrows/next.gif");a.setAnonymous(true);a.setEnabled(this.isEnabled());a.setParent(this);a._addCssClassName("QxMenuButtonArrow");};proto._innerWidthChanged=function(){this._d4Width();this._d1("inner-width");};proto._innerHeightChanged=function(){this._d4Height();this._d2("inner-height");};proto._d1=function(_e5){var vParent=this.getParent();if(this._f1){this._f1._d3Horizontal(vParent._childIconPosition);};if(this._f2){this._f2._d3Horizontal(vParent._childTextPosition);};if(this._f3){this._f3._d3Horizontal(vParent._childShortcutPosition);};if(this._f4){this._f4._d3Horizontal(vParent._childArrowPosition);};};proto._d2=function(_e5){var vInner=this.getInnerHeight();if(this._f1){this._f1._d3Vertical((vInner-this._f1.getPreferredHeight())/2);};if(this._f2){this._f2._d3Vertical((vInner-this._f2.getPreferredHeight())/2);};if(this._f3){this._f3._d3Vertical((vInner-this._f3.getPreferredHeight())/2);};if(this._f4){this._f4._d3Vertical((vInner-this._f4.getPreferredHeight())/2);};};proto._setChildrenDependHeight=function(_e4,_e5){if(this._c1&&_e4==this._f1&&_e5=="unload"){return true;};var newHeight=this._d5Height(_e4,_e5);if(this._heightMode=="inner"&&this._heightModeValue==newHeight){switch(_e5){case "load":case "append-child":case "preferred":switch(_e4){case this._f1:case this._f2:case this._hintObject:case this._f4:return this._d2(_e5);};};}else {this.setInnerHeight(newHeight,null,true);};return true;};proto.getNeededIconWidth=function(){return this._c1?this._f1.getAnyWidth():0;};proto.getNeededTextWidth=function(){return this._c2?this._f2.getAnyWidth():0;};proto.getNeededShortcutWidth=function(){return this._c3?this._f3.getAnyWidth():0;};proto.getNeededArrowWidth=function(){return this._c4?this._f4.getAnyWidth():0;};proto._g1=function(e){this.execute();};proto.dispose=function(){if(this.getDisposed()){return;};this.removeEventListener("mousedown",this._g1);return QxWidget.prototype.dispose.call(this);};