
//-----is there an easy adaptable way to have an automatic "commit" in addition
//    to save,load in the context of laxkit??

//Save
//Save As
//Commit...
//


//---Initial Commit
//
//Message: ______________-
//
//Start new repository:  svn  git
//Add to existing repository: url:/ao/ue/aoeuaoeu/ao/euao
//
//-for git:
//git init    ->  creates ./.git
//git add .   ->  add ref to everything in . to the .git thing
//git commit  ->  actually commit those things to rep (asks for message)
//
//
//git log --stat --summary
//
//git branch  -> show list of current branches
//git branch nameofbranch    -> create new branch
//git checkout nameofbranch  -> switch to that branch
//git merge nameofbranch     -> merge back to master
//git branch -d nameofbranch -> delete a branch, checking to see that changes are merged
//git branch -D nameofbranch -> delete a branch, not merging
//gitk  -> show graph of branches
//
//git clone ...  -> create a new working repository somewhere
//git pull ...   -> sync current working rep from another
//git fetch ...  -> like pull, but does not merge with current working rep
//git log -p HEAD..FETCH_HEAD  -> show changes with fetched stuff
//git log -p HEAD...FETCH_HEAD  -> show only different changes with fetched stuff
//
//git remote add bob /home/bob/myrepo
//git merge bob/master
//git pull . remotes/bob/master
//
//git show .....   -> show info about a commit
//git show HEAD
//git show HEAD^    -> one commit back
//git show HEAD^^   -> two commits back
//git show HEAD-4   -> 4 commits back
//git show HEAD^1   -> show first parent of HEAD
//git show HEAD^2   -> show second parent of HEAD
//
//git tag sometag ab342ce324323535     -> make an alias for a commit
//git tag -f sometag ab342ce324323535  -> force replace a tag
//
//git grep "hello" somecommitname -> search for things in any version, omit v name for current version
//
//git reset version   -> deletes changes after going back to version
//git revert version  -> doesn't delete changes after going back to version





//----Commit
//
//Message: ___________________
//
//Files:
//  doc1.laidout
//  doc2.laidout
//  doc3.laidout
//  map.project
//Repository: git:/aoeua/aoe/uaoe/ua/e
//			  svn:/oeu/ao/eua/oeua/o
//
//[Commit][Cancel]



//---Restore
//
//  rev 1
//  rev 2
//  rev 3
//  |    \
//rev 4a  rev 4b
//rev 5a  rev 5b
