# Social-Media-Network
Developed a social media platform in **C**, simulating user connections, post management, and personalized feeds using **graphs** and **trees**.  
The project allows users to add or remove friends, compute distances between users, create and repost content, like posts, view feeds, and even detect cliques (fully connected friend groups).  
The implementation focuses on modularity, efficient data structures, and correct memory management — all logic is handled programmatically with no GUI.

# How to Use the Social Media Platform

  ## Adding and Removing Friends
      Command: add <name1> <name2>
      Creates a bidirectional friendship between two users.

      Example: add Mihai Andrei
      Output: Added connection Mihai - Andrei

      Command: remove <name1> <name2>
      Removes the friendship between two users.

      Example: remove Mihai Andrei
      Output: Removed connection Mihai - Andrei

  ## Finding Distance Between Users
      Command: distance <name1> <name2>
      Calculates the distance between two users using BFS (Breadth-First Search).

      Example: distance Mihai Mihnea
      Output: The distance between Mihai - Mihnea is 3
      If no path exists: There is no way to get from Mihai to Mihnea

  ## Friend Suggestions
      Command: suggestions <name>
      Displays all “friends of friends” who are not already friends with the user.

      Example: suggestions Alex
      Output:
      Suggestions for Alex:
      Maria
      Mihai
      Vlad

  ## Common Friends
      Command: common <name1> <name2>
      Displays all mutual friends between two users.

      Example: common Alex Ana
      Output:
      The common friends between Alex and Ana are:
      Andrei
      Maria
      Ioana

  ## Counting and Ranking Friends
      Command: friends <name>
      Displays how many friends a user has.

      Example: friends Andrei
      Output: Andrei has 5 friends

      Command: popular <name>
      Displays the most popular user among the given user and their friends.

      Example: popular Mihai
      Output: Mihai is the most popular

  ## Creating a Post
      Command: create <username> "<title>"
      Creates a new post with a unique ID.

      Example: create Mihai "Titlu postare"
      Output: Created "Titlu postare" for Mihai

  ## Reposting Content
      Command: repost <username> <post-id> [<repost-id>]
      Creates a repost for a post or another repost.

      Example: repost Alex 1 2
      Output: Created Repost #10 for Alex

  ## Finding Common Reposts
      Command: common-repost <post> <repost1> <repost2>
      Finds the first common repost (Lowest Common Ancestor) between two reposts.

      Example: common-repost 1 4 7
      Output: The first common repost of 4 and 7 is 2

  ## Likes
      Command: like <username> <post-id> [<repost-id>]
      Toggles a like for a post or repost. If the same user likes again, it removes the like.

      Example:
      like Alex 0
      Output: Alex liked "Titlu postare"
      like Alex 0
      Output: Alex unliked "Titlu postare"

  ## Ratio
      Command: ratio <post-id>
      Compares likes between a post and its reposts to see which has higher engagement.

      Example: ratio 0
      Output: Post 0 got ratio'd by repost 1
      If the original post has more likes:
      Output: The original post is the highest rated

  ## Deleting Posts
      Command: delete <post-id> [<repost-id>]
      Deletes a post or repost and all its dependent reposts.

      Example: delete 0
      Output: Deleted "Titlu postare"

  ## Viewing Likes and Reposts
      Command: get-likes <post-id> [<repost-id>]
      Displays the number of likes for a post or repost.

      Example:
      get-likes 1 2
      Output: Repost #2 has 5 likes

      Command: get-reposts <post-id>
      Displays all reposts under a post or repost in hierarchical order.

      Example:
      get-reposts 1
      Output:
      "Cat video" - Post #1 by Alex
      Repost #2 by Alex
      Repost #4 by Alex
      Repost #5 by Alex
      Repost #7 by Alex
      Repost #3 by Alex
      Repost #6 by Alex

  ## Feed
      Command: feed <username> <feed-size>
      Displays the most recent posts made by the user and their friends.

      Example:
      feed Andrei 5
      Output:
      Luca: "Nu-mi vine sa cred cine a castigat Grand Prix-ul de la Miami"
      Ana: "Ati auzit ultima melodie a lui Kanye?"
      Alex: "Buna TPU, merita sa dau la Poli?"
      Mihnea: "Vand Golf 4"
      Mihai: "Al doilea"

  ## Viewing Profile
      Command: view-profile <username>
      Displays all posts and reposts made by a specific user.

      Example:
      view-profile Andrei
      Output:
      Posted: "Prima mea postare"
      Reposted: "Shaorma e veatza mea"

  ## Friends That Reposted
      Command: friends-repost <username> <post-id>
      Displays which of the user’s friends reposted a specific post.

      Example:
      friends-repost Andrei 5
      Output:
      Friends that reposted:
      Mihai
      Alex
      Ana

  ## Clique Detection
      Command: common-group <username>
      Finds the largest friend group (clique) that includes the specified user, using the Bron–Kerbosch algorithm.

      Example:
      common-group Alex
      Output:
      The closest friend group of Alex is:
      Alex
      Ana
      Ioana
      Mihai

  ## Exiting the Program
      Command: exit
      Ends the program, frees all allocated memory, and stops execution.

      Example: exit
      Output: (no message shown)

# Brief presentation of the implementation

This project implements a social network simulation using graphs and trees. Below is an explanation of how each feature is implemented:

  ## Friendship Management
  Friendships are stored in adjacency lists, with bidirectional links between users.  
  Adding or removing a friend updates both lists.  
  The distance between users is calculated using BFS traversal of the friendship graph.  
  Functions like `common`, `popular`, and `suggestions` rely on adjacency traversal and sorting for display.

  ## Post and Repost System
  Posts and reposts are represented as nodes in a tree.  
  Each node stores:
  - A unique ID
  - Author name
  - Title (NULL for reposts)
  - Like count
  - List of child reposts

  This structure allows hierarchical reposting, quick traversal, and efficient deletion of subtrees.

  ## Feed Generation
  The feed merges the posts of a user and their friends, sorted by recency.  
  Implemented with linked lists and efficient merging, ensuring linear-time operations even with large inputs.

  ## Clique Detection
  The largest fully connected friend group is detected using the Bron–Kerbosch recursive algorithm.  
  The result includes the queried user and all mutually connected friends.

  ## Likes and Ratio
  Each post/repost maintains an internal counter for likes.  
  The `ratio` function compares the original post with all reposts, displaying which one has higher engagement.  
  In case of equality, the post with the lower ID is preferred.

  ## Memory Management
  All dynamically allocated structures (graphs, trees, lists) are freed at program termination.  
  Valgrind checks confirm no memory leaks after extensive testing.

  ## Modularity and Error Handling
  Each major component (friends, posts, feed, users) is implemented in a separate C file.  
  The main file reads input commands using `fgets` and delegates execution.  
  Invalid commands are handled gracefully, printing descriptive error messages.

  ## Checker
  A custom automated checker was implemented to validate functionality.  
  It includes over **50 automated tests**, checking:
  - Graph correctness
  - Repost tree structure
  - Feed generation
  - Ratio accuracy
  - Memory safety (validated with Valgrind)

  The checker supports both GUI and CLI versions:
