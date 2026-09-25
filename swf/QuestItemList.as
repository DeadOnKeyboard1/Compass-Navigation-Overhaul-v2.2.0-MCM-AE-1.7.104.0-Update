var questItem:QuestItem;
var entries:Array;

var All:Boolean;
var Favor:Boolean;
var StealthMode:Boolean;
var Swimming:Boolean;
var HorseMode:Boolean;
var WarHorseMode:Boolean;

var positionY0:Number;
var maxHeight:Number;

var SCALE:Number = 65;

function ResolveHUDMenu():MovieClip
{
	var current:MovieClip = this;
	for (var i:Number = 0; i < 10 && current != undefined; i++)
	{
		if (current.HudElements != undefined || current.CompassMarkerList != undefined)
		{
			return current;
		}
		current = current._parent;
	}
	if (_root.HUDMovieBaseInstance != undefined)
	{
		return _root.HUDMovieBaseInstance;
	}
	return _root;
}

function ResolveCompassHolder(a_hud:MovieClip):MovieClip
{
	if (a_hud != undefined && a_hud.__CNO_CompassHolder != undefined)
	{
		return a_hud.__CNO_CompassHolder;
	}
	if (a_hud != undefined && a_hud.CompassShoutMeterHolder != undefined)
	{
		return a_hud.CompassShoutMeterHolder;
	}
	return undefined;
}

function QuestItemList(a_positionX:Number, a_positionY:Number, a_maxHeight:Number):Void
{
	entries = new Array();

	positionX0 = Stage.width * a_positionX;
	positionY0 = Stage.height * a_positionY;

	var point:Object = { x:positionX0, y:positionY0 };
	globalToLocal(point);
	_x = point.x;
	_y = point.y;

	maxHeight = Stage.height * a_maxHeight;

	// Show except in dialogue mode
	All = true;
	Favor = true;
	StealthMode = true;
	Swimming = true;
	HorseMode = true;
	WarHorseMode = true;
}

function AddToHudElements():Void
{
	var hud:MovieClip = ResolveHUDMenu();
	if (hud != undefined && hud.HudElements != undefined)
	{
		for (var i:Number = 0; i < hud.HudElements.length; i++)
		{
			if (hud.HudElements[i] == this)
			{
				return;
			}
		}
		hud.HudElements.push(this);
	}
}

function AddQuest(a_type:Number, a_title:String, a_isInSameLocation:Boolean, a_objectives:Array, a_ageIndex:Number):Void
{
	questItem = attachMovie("QuestItem", "questItem", getNextHighestDepth(), { _xscale:SCALE, _yscale:SCALE });

	entries.push(questItem);

	questItem.SetQuestInfo(a_type, a_title, a_isInSameLocation, a_objectives, a_ageIndex);
	questItem.gotoAndStop("IdleHide");
}

function SetQuestSide(a_side:String):Void
{
	questItem.SetSide(a_side);
}

function Update():Void
{
	// iHUD / compass-toggle compatibility. Unknown HUD layouts default to visible
	// instead of hiding the quest list permanently.
	var hud:MovieClip = ResolveHUDMenu();
	var holder:MovieClip = ResolveCompassHolder(hud);
	var compassVisible:Boolean = true;
	if (holder != undefined)
	{
		if (holder._alpha != undefined && holder._alpha <= 0)
		{
			compassVisible = false;
		}
		if (holder.Compass != undefined && holder.Compass.DirectionRect != undefined &&
			holder.Compass.DirectionRect._alpha != undefined && holder.Compass.DirectionRect._alpha <= 0)
		{
			compassVisible = false;
		}
	}

	if (compassVisible)
	{
		if (entries.length > 1)
		{
			entries.sort(ByAgeThenMiscellaneousQuests);

			var yOffset = 0;
			for (var i:Number = 0; i < entries.length; i++)
			{
				questItem = entries[i];

				questItem._y = yOffset;

				var point:Object = { x:0, y:0 };
				questItem.localToGlobal(point);

				if (point.y >= (positionY0 + maxHeight))
				{
					questItem._visible = false;
				}
				else if (questItem.ObjectiveItemList.length > 1)
				{
					for (var j:Number = 1; j < questItem.ObjectiveItemList.length; j++)
					{
						var objectiveItem:MovieClip = questItem.ObjectiveItemList[j];

						var point:Object = { x:0, y:0 };
						objectiveItem.localToGlobal(point);

						if (point.y >= (positionY0 + maxHeight))
						{
							objectiveItem._alpha = 0.0;
						}
					}
				}

				yOffset += questItem._height + 5;
			}
		}

		_alpha = 100;
	}
	else
	{
		_alpha = 0;
	}
}

function ShowQuest():Void
{
	questItem.Show();
}

function RemoveQuest():Void
{
	questItem.Remove();
}

function ShowAllQuests():Void
{
	for (var i:Number = 0; i < entries.length; i++)
	{
		questItem = entries[i];

		if (!questItem.isBeingShown)
		{
			questItem.Show();
		}
	}
}

function RemoveAllQuests():Void
{
	for (var i:Number = 0; i < entries.length; i++)
	{
		questItem = entries[i];
		if (!questItem.isBeingShown)
		{
			questItem._alpha = 0;
		}
		questItem.Remove();
	}

	entries.splice(0, entries.length);
}

function ByAgeThenMiscellaneousQuests(a_questItem1:QuestItem, a_questItem2:QuestItem):Number
{
	if (a_questItem1.TitleEndPiece._currentframe != QuestItem.miscQuestFrame &&
		a_questItem2.TitleEndPiece._currentframe == QuestItem.miscQuestFrame)
	{
		return -1;
	}
	else if (a_questItem1.TitleEndPiece._currentframe == QuestItem.miscQuestFrame &&
			 a_questItem2.TitleEndPiece._currentframe != QuestItem.miscQuestFrame)
	{
		return 1;
	}
	else if (a_questItem1.ageIndex > a_questItem2.ageIndex)
	{
		return -1;
	}
	else if (a_questItem1.ageIndex < a_questItem2.ageIndex)
	{
		return 1;
	}

	return 0;
}
