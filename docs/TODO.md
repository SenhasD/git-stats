<!--toc:start-->
- [FEATURES](#features)
- [OVERALL LOGIC](#overall-logic)
- [NOTES](#notes)
- [ASSUMPTIONS](#assumptions)
- [TIMELINE](#timeline)
- [TODO](#todo)
- [Formatting TODO](#formatting-todo)
- [Optimistic](#optimistic)
<!--toc:end-->

## FEATURES

- ~~specify multiple repos~~
- ~~set refresh rate~~
- ~~untracked files~~
- ~~plus lines && minus lines~~
  - ~~enable~~
  - ~~change font~~
  - ~~font size~~
  - ~~color~~
  - ~~number only~~
- ~~text will always be centered~~
  - ~~or this could also be a more creative way like every 1k put an indicator somewhere~~
- ability to convey to the user useful error outputs
  - for invalid paths coming from the config

## OVERALL LOGIC

- get the output of the diff command & store it  
- add all of the collective +'s and -'s together to get the sum that should be outputted
- parse through it to get the data in a segmented form
- depending on the settings output it in a specific format

## NOTES

- DO NOT WORRY ABOUT WINDOWS THIS IS FOR YOU ATM

## ASSUMPTIONS

- ~~when grabbing input from the obs ui/user the output of that will be an array
of strings~~
  - this assumption was wrong! It's one big string that can be delimited
  by newline characters~
- ~~if we have the string "insertions" within the output of git diff then we
have a full valid output~~
  - this assumption was also wrong! for git diff you can have just deletions,
  just insertions, or both. In addition to this the word also changes
  depending on if its plural or not!~
- ~~we are expecting the end directory to have the / at the end~~
  - not a necessary assumption as it can just be added at the end~
- no one in their right mind would have either 4 mil insertions or deletions

## TIMELINE

- ~~create the utility using purely the command line~~
- ~~fix building of the plugin~~
- slap that bad boi in obs form
- $$profit$$

## TODO

- ~~get git diff info for file specified by me~~
- ~~add '/' to the end of all paths if they do not already have it~~
- ~~take a list of paths and get the git diff info from all of them~~
- ~~take out insertions/deletions from entire shortstat line~~
  - ~~cut up the line using the commas as delimiters~~
    - ~~using that cut up section grab the long from it~~
- ~~add all of those together~~
  - ~~use a long for this~~

- ~~REWRITE THE ENTIRETY OF THE BASE~~
  - ~~interface with the freetype2 text source itself~~
  - ~~get the source to showup correctly~~
  - ~~get the rendering working correctly~~
  - ~~get the updating fixed~~
- ~~create module for hash table implementation~~
  - ~~create submodule for hash table input~~

- ~~test updateValue in the hash table~~
- NOTE: SPLIT THIS UP INTO EACH PART AND CALL THEM FROM THE SOURCE FILE
  - as you've seen it gets very messy if doing everything in the source file

- ~~decide if I want hash table or hash map (leaning towards map)~~
- ~~put two hash tables or maps inside of the obs source data structure to
represent both tracked and untracked files~~
  - ~~replaced this with one hashmap and an array of paths because tracked
  files only need the paths~~
- ~~update hashtable and hashmap to be taking untracked files instead of
boarding passes~~
- ~~update the CMakeLists file to include the hashmap library~~
- ~~get rid of the following properties for the text source:~~
  - ~~text~~
  - ~~read from file~~
  - ~~chat log mode~~
  - ~~chat log lines~~
  - ~~text file~~
- ~~when calling the update function for the source, when gettings the repos you must delimit the string by newlines in order to get the paths~~
- ~~we interface with both or one of these handlers depending on what the
use set the settings to~~
  - ~~ex. if the user does not want untracked files then we would never update
  the untracked portion of the data~~
- ~~expand the ~ into an absolute path~~
- ~~figure out why we get garbage for output when we have a valid path
(working when path is invalid)~~
- ~~fix the untracked files not going into the hashmap~~
- ~~get the updated text working properly~~
  - ~~the git repos need to be accurately found~~
  - ~~updates need to happen after this~~
- ~~fixed the order of the numbers outputted from ltoa~~
- ~~make it so that the command checker does not run if the file is not valid~~
- ~~add only ticking when timer is done~~
- ~~fix the typing issue~~
  - ~~crashes when typing out the repo path~~
- ~~change delay to an integer~~
- ~~create handlers for both untracked and tracked files
(THIS IS GOING TO BE IN THE UPDATE FUNCTION)~~
  - ~~for tracked you just need to get the info from the git diff output~~
  - ~~for untracked you need to actually go through and manage those~~
- ~~in updateUntrackedFile fix time issue (timing is not correct on the checking
if the file has been edited)~~
- ~~make a function that goes through the untracked files and deleted ones
that should not be there anymore~~
  - ~~do this by checking if a repo path is a substring in the entire path
  of the untracked file~~
    - this was ignored because it its more expensive to check all keys for the substring
    than it is to just redo the hashmap
- ~~make the plugin run off rip (shouldn't have to update something
before getting output)~~
- ~~make a clear hashmap function and use it at update so that untracked files
can be live updated as well~~
  - basically implemented when hashmap was recreated
- ~~allow updating of the source in the update function just don't update
the contents of it in relation to the plugin~~
- ~~make untracked files not accounted for when unticking untracked
files bool from a ticked state~~
- ~~see if its possible to use restrict keyword to fix pointer issue for -O0 compilation~~
- ~~reorder the properties so that the text properties are at the bottom~~
  - ~~instead use a group to better categorize the options
  (unless I find a way to reorder)~~
    - ~~changed this to be both at the same time. Grouping is put into place
    and order has been changed to put git-stats settings on top~~
- ~~change the single repo list to this:  ~~
  - ~~<https://docs.obsproject.com/reference-properties?highlight=properties#c.obs_properties_add_editable_list>~~
  - this does not give me the functionality I want as I am working with
  dirs not files
- ~~figure out how to allow directories to be specified that hold multiple repos~~
  - ~~make this a separate field~~
  - ~~use this to make it purdyy~~
    - ~~<https://docs.obsproject.com/reference-properties?highlight=properties#c.obs_properties_add_path>~~
  - ~~check if a directory already exists in the tracked paths array before adding it via addGitRepoDir~~
- ~~fix untracked file pathing~~

- ~~implement git diff into the plugin~~
  - ~~reformat git-diff implementation to be a module and not a run file itself~~
  - ~~add the ability to get untracked files~~
  - ~~use this command to get the file path of the untracked files ```git ls-files --others --exclude-standard```~~
  - ~~given the output of this command count the number of lines in that file at the current moment and store it in a hash table~~
  - ~~add this to the count of insertions/deletions (depending on the original value)~~
  - when getting these untracked files ignore executables
- ~~look into clarity of text source~~
- ~~split up insertions and deletions into two different text sources~~
  - in tick split up the string handling into insertions and deletions and pass those to their respective text sources
  - ~~in update we need to somehow change the settings of one source and then pass that onto the second one as well~~
    - ~~changing to both having their own unique settings~~
      - ~~the only truly unique settings are going to be the colors. The rest are going to be linked~~
- ~~allow enabling and disabling of insertion deletions~~
- ~~create overflow indicators to give constraints without editing the underlying text source~~
  - ~~CURRENT IDEA:~~
    - ~~max both insertions and deletions to 9999 each~~
    - ~~each time the number gets above that threshold put a period over the number (max of 4)~~
    - ~~use monospace/nerdfont for this~~
      - ~~see if there is a way to force monospace fonts~~
        - just tell the user to use one LOL
    - ~~manage spaces in between the numbers~~
      - ~~example:~~
        - ~~put one space in between +9999 and -9999 with the four dots above the numbers~~
        - ~~when plugin overflows one of these values add spaces in between insertions and deletions to keep spacing consistent~~
- ~~ensure that width and height are consistent and make sense~~
  - ~~not going to make the width the same at all times but rather just give the user the ability to "test" it on their own to get the width right~~
- ~~make NULL output +0 -0 not ""~~
- ~~make tracked paths (the ones specified by the user in the block) reset to 0 when they're deleted~~
- ~~make default for both groups enabled~~
- ~~for default properties figure out a way to make insertions green and deletions red~~
- ~~make a "test" button that ill just show the maximum values~~
- ~~do not allow for duplicate repos~~
  - ~~check in the user specified strings that it doesn't already exist~~
  - ~~check when creating the directory thingy that it doesn't already exist~~
- ~~allow the user to get rid of the + and - signs~~
  - ~~account for spacing with this~~
- ~~make counter not reset to 0 0 after updating props~~
- ~~make the colors apply on startup~~
  - ~~at the current moment when you boot up obs it uses defaults~~
- ~~make default for + symbol ticked~~
- ~~figure out why on startup only the insertion is visible~~
- ~~fix seg fault on destroy~~
- ~~change insertion source name to insertion source (current it is called text source)~~
  - ~~change the insertion group name to insertion_properties (currently it is text_properties)~~
- ~~figure out a way to do popups or something else for the user so they know when errors have happened~~
  - ~~checkout this: <https://docs.obsproject.com/reference-properties#c.obs_property_text_set_info_type>~~
    - continue using the callback, but see if it doesn't reset cursor back to beginning if you pass in a different group to the setter for the callback
    - ~~still need to get the error message to show up~~
    ~~- oh yeah and also remove the testing stuff for hte callback~~
  - ~~current idea is to use the info string type for the properties, but need to figure out how to update the property~~
  - tell user its actually an error (prepend INVALID: to the error string)
  - **MAJOR CHANGE OF PLANS:** going to attempt to use an editable list instead so that errors theoretically don't need to be passed to the user
- ~~figure out how to the editable list works and how to implement that into the current solution~~
  - ~~figure out how to add the property~~
  - ~~figure out how to get the strings from the property~~
  - ~~change up the update function depending on how the data is now formatted~~
- ~~figure out why last number of deletions flickers~~
  - cannot reproduce at all come back to this if its still an issue
- ~~don't allow overflow indicators to go over 4~~
- ~~fix inconsistent spacing when using different modes/settings~~
- ~~fix performance issues~~
- ~~add custom icons for the overload indicator~~

- get rid of memory leaks
- make update not reset it to 0 0
- figure out how to use gh actions to build
    - seems like I just have to turn them on because they are not automatically turned on when you fork a repo 
- test the build on a different env (prolly arch peecee)

## Refactor Notes

## Formatting TODO

- ~~pick either camel case or snake case~~
  - ~~camel case is personally preferred but it is probably easier to do snake since thats what all of the obs functions are in~~

## Optimistic

~~- debug the need to run with -O0~~

- ~~add ability to change overflow icon~~
  - ~~USE THIS TO EXTRACT THE FIRST UNICODE CHARACTER:~~

    ```C
      #include <string.h>
      #include <locale.h>
      #include <uchar.h>
      char *test = "\uf066";
      char buff[1000] = {0};
      char32_t specChar;

      mbstate_t mbs;
      setlocale(LC_ALL, "");
      memset(&mbs, 0, sizeof(mbs));
      size_t size = mbrtoc32(&specChar, test, 16, &mbs);
      c32rtomb(buff, specChar, &mbs);
      printf("%s\n", buff);
    ```

    - ~~figure out why the overload isn't taking the unicode character properly~~

- refactor to make things more concise
- make it blazingly fast
- implement profiler or at least learn how it works
- mess around with p_threads

## DEBUG NOTES

- ~~currently running obs normally seg faults but running with the -O0 and the following valgrind command:~~
  - doesn't seg fault anymore with optimizations on!

```
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./executable exampleParam1

```

LOG_ERROR: for errors that don't require the program to exit
LOG_WARNING: when error occurs and is recoverable
LOG_INFO: info for whats going on
LOG_DEBUG: use for debug //// only sent when debug is true

### Psuedo for updating the deletion source

- ~~create properties managed by me,~~
- ~~in update use ```obs_data_set_blah``` to set the properties of the source~~
  - ~~to get the value to set it to use obs_data_get_blah on settings~~

### Psuedo For Dirs

** NOTE: THIS IS RUN ONLY ON UPDATE AFTER THE UPDATING AND CLEARING OF THE TRACKED PATHS **

- ~~grab repo dir from properties~~
- ~~verify that repo exists~~
- ~~grab all of the dir strings within the repo~~
- ~~verify that all of these dirs are git repos~~
  - ~~verify that the dirs don't already exist~~
  - ~~if is valid git repo add to tracked paths~~
  - ~~else skip this dir~~

## README 
- for ideal spacing you want a monospaced nerd font

