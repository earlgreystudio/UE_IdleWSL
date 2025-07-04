Advanced RPG System (ARS) is an Unreal Plugin that allows developers to easily built complex RPG Systems.

Being based on Components, ARS is totally system agnostic and 100% Plug and Play.

You'll be able to define your own Attributes (like Strength, Dexterity, Intelligence etc.) your own Statistics (Health, Stamina, Mana etc.) and your Parameters (Melee Damage, Fire Damage, Crit Chance, Melee Defense etc).

ARS allows developers to define their very own rules on how each Attributes can affect the others through the use of float Curves, for example you can define how your Entities Constitution affects his own MaxHealth.

ARS comes with an Attributes Modifier system to temporarily or permanently apply modifications on your AttributeSet at runtime.

You can select between 2 different Level-up systems: an automatic one in which Attributes on level up are incremented through the use of curves and a Perk-based, in which at every level your Entity gains a certain amount of perks that can be used to increase his Attributes.

Advanced RPG System is built with Multiplayer in mind, so everything is efficiently replicated.

The system is written in C++ for efficiency, but all is exposed to Blueprints so that you can use it without writing a single line of code.

Advanced RPG System

Advanced RPG System (ARS) is one of the modules of ACF. It handles all the “RPG side”of your game. But let’s start from the beginning. 



2.1 Statistics, Attributes and Primary Attributes

ARS supports three different kind of Stats, in particular: 

Primary Attributes: like Strength, Endurance, Constitution etc. those are used to automatically generate your Statistics and Attributes

Attributes: like Attack Damage, Defense, Crit Chance etc. those are characterized by a Value that is use for your actual calculation

Statistics: like Health, Stamina, Mana etc. those are characterized by a Max Value, a Current Value and a Regeneration Value





2.2 Plugin Setup

The first to do is to create and setup your own AttributeSet. This can be done through a DataTable (Right Click on Content -> Miscellaneous -> DataTable).

The type of struct used for the DataTable should be GamePlayTagTableRow (the one highlighted in the image below). 

In  that DataTable you could actually define the GameplayTags that identifies your AttributeSet. They should be written in the form:

RPG.Statistic.Health

Where RPG is the global tag root, Statistic identifies the stat type (Statistic / Attribute / Primary Attributes) and Health is the actual Statistic. In this Datatable you can actually setup your stat list. To use those GameplayTag we’ll need to go in to our GameplayTag Settings





(Edit -> Project Settings -> GameplayTags) and ad the tags created by adding the DataTable in the GameplayTagTableList.





Now your GameplayTags are effective and we can switch to the actual Stat configuration.

Please notice that you cannot add GameplayTags at Runtime!



2.3 AttributeSet Setup


This step can only be done if you have successfully set your GameplayTags. In your ProjectSettings tab, locate the Ascent RPG Settings. Here you have to tell to the Plugin the actual Tag Roots of your AttributeSet. For Example: 

If RPG.Statistic.Health is one of your statistics, RPG.Statistic will actually be the Root of all you Statistics tag. This setup is necessary since the Plugin ensures every time you are using a wrong Tag as a parameter for a method or to prevent errors during a Character configuration. 

ARS allows the Designer to control all the character AttributeSet by just settings their Attributes and a set of Generation Rule that are used to automatically generate all the Statistics and Attributes.



To Set your generation Rules you’ll need to create a DataAsset of the type ARSGenerationRulesDataAsset by right clciking in content browser -> Miscellaneous -> Data Asset.





Generation Rules are set through the use of UCurveFloat objects. You can create your own curves in any folder (Right Click in

Content Browser ->Miscellaneous -> Curve). In the image below how the RPG.PrimaryAttribute.Strength have been setup to modify the RPG.Attribute.MeleeDamage . 





In this case we can see how, if the strength of our Character is 13, his bonus to MeleeDamage will be 96.86. Please notice that every PrimaryAttribute can modify multiple Statistics/Attributes and Statistics/Attributes can be modified by multiple Attributes, in that case the actual value will be the sum of all the influences. The same thing must be done for each Statistics with the only difference tha Statistics have two different values that can be modified: Max Value and Regeneration Value.









In the above sample RPG.Attribute.Endurance have been set to generate both Max Stamina and Stamina Regeneration (RPG.Statistic.Stamina).



Once finished remember to put your Data Asset in Project Settings -> Ascent RPG System.





2.4 ARS Component Configuration
To actually implement ARS functionality you must add the ARS Statistics Component to your agent and configure it. ARS supports different ways to handle your AttributeSet. To properly use the Plugin is important to understand how they work.

.



StatisticConsumptionMultiplier: Multiplier applied every time you modify CurrentValue of the target Statistics. Could be useful to implement logics like: if your inventory is full, Stamina consumption is multiplied by 1.5 for every action.



StatsLoadMethod: Define how your Statistics and Attributes are generated: 

Default Without Generation: No generation is applied, Default values are used.

Generate From Default Attributes: Define your Attributes in DefaultAttributeSet, Attributes and Statistic will be generated following the rules defined in ProjectSettings ARS Settings.

Load By Level From Curve: Define the level of the character and Generate AttributeSet from AttributesByLevelCurves. 

Default value is set to Generate From Default Attributes.

DefaultAttributeSet: Attribute set used if you select No Generation or Generate From Default Values. In the first case it is used “as is”, in the second one, your Attribute will be used to generate your Statistics and Attributes following your previously configured ARS Settings.

CanRegenerateStatistics: This defines if the Statistics of this Character can regenerate. Turn off regeneration for performance optimization. 

RegenerationTimeInterval: ARS Component does not tick for performance reasons. This value defines the time interval for regeneration. Set high values for optimization. 

To actually start to work with ARS, remember to call the InizializeAttributeSet in your beginplay (or when you want to start to work with it) - (NOTE: this is not needed in ACF since it’s done automatically)



2.5 Attribute Modifiers
AttributeModifiers are used to apply modifications to the AttributeSet of your Characters. This could be useful if you want, for instance, that your Attributeset could be modified by your equipment or by a particular state. There are two different types of Modifiers: Additive and Percentage. Additive modifiers are simple additions of the actual values of the modifier to the actual values of the Character AttributeSet. Percentage modifiers apply a modification based on the current values of the AttributeSet (20 means that the actual value of that stat will be raised by 20%).

This particular modifiers increments MaxStamina by 20, Stamina regeneration by 5 and Ranged Damage by 75. Modifiers can be added and removed at runtime by simply calling the relative methods:









Modifiers are always handled Server side. You can also use Timed modifers, who actually are automatically removed after a certain amount of time. All those functions are exposed to Blueprints.



Please notice that Modifiers have no effects for Statistics Current Values, since they are instantly normalized to the new MaxValue. If you actually want to modify CurrentValue of a Statistic you must use:



Statistic modification is always handled Server side.

2.6 Experience and Levelling
ARS includes a leveling system. To make it work properly you also have to setup your ExpForNextLevelCurve. Then you can the BP exposed function: 



That will add the experience and eventually level up your character.

 You can select it by changing the Leveling type value: you can choose to not use it all, to use a curve based leveling system or a perk based leveling system. 



CharacterLevel: Character Level used for generation if leveling system is active

ExpForNextLevelCurve:  For Each level the amount of Exp necessary to Lvl Up set as UCurveFloat. X axes is Character’s current level, and Y axis is the amount of exp to level up. 



2.6.1 PerkBased Leveling System
If you select the PerkBased LevelingSystem at every level up your character will receive PerksObtainedOnLevelUp amount of perk and you have to manually call the AssignPerkToAttribute function to consume a numPerk number of perks, modify an attribute and regenerate the AttributeSet of the character.the curve based leveling system you







2.6.2 CurveBased Leveling System
With the curve based leveling system you have to setup a set of curves that will be used to automatically regenerate your character's stat at each level up. 

AttributesByLevelCurves: Implement Rules to generate your ATTRIBUTES starting from your Level. For each Curve on X you have the Level, on Y the actual value of the Attribute for that Level. This is only used if the StatsLoadMethod have been set on Load By Level From Curves. In that case your Attributes will be read from your UCurveFloat in which in the X Value is the Character Value and the Y value is your actual Attributevalue. Statistics and Attributes will be generated consequently following your previously configured ARS Settings.



2.7 Blueprint Functions
Inside ARS you’ll find a DocuActor which includes an overview of all the BP functions of the plugin.



2.8 Delay
In latest ARS version a regeneration delay can be setted for each statics. This is used to replicate most of the stamina based mechanics in which, after you perform an action that consumes your Statistic, you regeneration