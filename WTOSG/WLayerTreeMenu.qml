import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import WTOSG

Menu{
	id:root
	property string text:""
	property bool highlighted:true

	delegate: MenuItem{
		id:menuItem
		contentItem:Item{
			Text{
				anchors.verticalCenter:parent.verticalCenter
				anchors.left:parent.left
				anchors.leftMargin:5
				text:menuItem.text
				color:enabled?CommonDefines.text:CommonDefines.disabledTest
			}
			Rectangle{
				id:indicator
				anchors.verticalCenter:parent.verticalCenter
				anchors.right:parent.right
				width:6
				height:parent.height
				visible:menuItem.highlighted
				color:CommonDefines.color2
			}
		}
		background:Rectangle{
			implicitWidth:210
			implicitHeight:35
			color:menuItem.highlighted? CommonDefines.active : "trasparent"
		}
	}

	background:Rectangle{
		implicitWidth:210
		implicitHeight:35
		color:CommonDefines.surface2
	}
}