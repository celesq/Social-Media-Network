# Social-Media-Network
I developed the core functionality of a social media platform in **C**, implementing user connections, posts, reposts, likes, feeds, and community detection using **graphs**, **trees**, and **linked lists**.  
The project leverages **graph-based modeling**, with algorithms such as **Lowest Common Ancestor (LCA)** for repost hierarchy traversal and **Bron–Kerbosch** for clique (community) detection.  
It was designed with scalability in mind, capable of handling datasets of over **100 users**, **500 friendships**, and **300 posts**, maintaining consistent performance and correctness.  
All functionality is implemented programmatically, without any GUI, emphasizing efficient data structures, modular architecture, and precise memory management.

# How to Use the Social Media Platform

  ## Adding and Removing Friends
      Command: ADD <user1> <user2>
      Creates a bidirectional friendship between two users.

      Example: ADD Michael Andrew
      Output: Added connection Michael - Andrew

      Command: REMOVE <user1> <user2>
      Removes the friendship between two users.

      Example: REMOVE Michael Andrew
      Output: Removed connection Michael - Andrew

  ## Finding Distance Between Users
      Command: DISTANCE <user1> <user2>
      Calculates the number of friendship links between two users using BFS (Breadth-First Search).

      Example: DISTANCE Michael Matthew
      Output: The distance between Michael - Matthew is 3

      If no connection exists:
      Output: There is no way to get from Michael to Matthew

  ## Friend Suggestions
      Command: SUGGESTIONS <user>
      Displays all “friends of friends” who are not already friends with the given user.

      Example: SUGGESTIONS Alex
      Output:
      Suggestions for Alex:
      Maria
      Mark
      Victor

  ## Common Friends
      Command: COMMON <user1> <user2>
      Displays all mutual friends between two users.

      Example: COMMON Alex Anna
      Output:
      The common friends between Alex and Anna are:
      Andrew
      Maria
      John

  ## Counting and Ranking Friends
      Command: FRIENDS <user>
      Displays the number of friends for a user.

      Example: FRIENDS Andrew
      Output: Andrew has 5 friends

      Command: POPULAR <user>
      Displays the most popular user among the given user and their friends.

      Example: POPULAR Michael
      Output: Michael is the most popular

  ## Creating a Post
      Command: CREATE <username> "<title>"
      Creates a new post with a unique ID.

      Example: CREATE Michael "My first post"
      Output: Created "My first post" for Michael

  ## Reposting Content
      Command: REPOST <username> <post-id> [<repost-id>]
      Creates a repost for a post or another repost.

      Example: REPOST Alex 1 2
      Output: Created Repost #10 for Alex

  ## Finding Common Reposts
      Command: COMMON-REPOST <post> <repost1> <repost2>
      Finds the first common repost (Lowest Common Ancestor) between two reposts.

      Example: COMMON-REPOST 1 4 7
      Output: The first common repost of 4 and 7 is 2

  ## Likes
      Command: LIKE <username> <post-id> [<repost-id>]
      Toggles a like for a post or repost. If the same user likes again, the like is removed.

      Example:
      LIKE Alex 0
      Output: Alex liked "My first post"
      LIKE Alex 0
      Output: Alex unliked "My first post"

  ## Ratio
      Command: RATIO <post-id>
      Compares likes between a post and its reposts to determine which has more engagement.

      Example: RATIO 0
      Output: Post 0 got ratio'd by repost 1

      If the original post has more likes:
      Output: The original post is the highest rated

  ## Deleting Posts
      Command: DELETE <post-id> [<repost-id>]
      Deletes a post or repost and all of its dependent reposts.

      Example: DELETE 0
      Output: Deleted "My first post"

  ## Viewing Likes and Reposts
      Command: GET-LIKES <post-id> [<repost-id>]
      Displays the number of likes for a post or repost.

      Example:
      GET-LIKES 1 2
      Output: Repost #2 has 5 likes

      Command: GET-REPOSTS <post-id>
      Displays all reposts under a post or repost in hierarchical order.

      Example:
      GET-REPOSTS 1
      Output:
      "Cat video" - Post #1 by Alex
      Repost #2 by Alex
      Repost #4 by Alex
      Repost #5 by Alex
      Repost #7 by Alex
      Repost #3 by Alex
      Repost #6 by Alex

  ## Feed
      Command: FEED <username> <feed-size>
      Displays the most recent posts made by the user and their friends.

      Example:
      FEED Andrew 5
      Output:
      Luke: "I can’t believe who won the Grand Prix in Miami"
      Anna: "Have you heard Kanye’s new song?"
      Alex: "Hey TPU, should I apply to Poli?"
      Matthew: "Selling my Golf 4"
      Michael: "Second one!"

  ## Viewing Profile
      Command: VIEW-PROFILE <username>
      Displays all posts and reposts made by a specific user.

      Example:
      VIEW-PROFILE Andrew
      Output:
      Posted: "My first post"
      Reposted: "Shawarma is life"

  ## Friends That Reposted
      Command: FRIENDS-REPOST <username> <post-id>
      Displays which of the user’s friends reposted a specific post.

      Example:
      FRIENDS-REPOST Andrew 5
      Output:
      Friends that reposted:
      Michael
      Alex
      Anna

  ## Clique Detection
      Command: COMMON-GROUP <username>
      Finds the largest friend group (clique) that includes the specified user, using the Bron–Kerbosch algorithm.

      Example:
      COMMON-GROUP Alex
      Output:
      The closest friend group of Alex is:
      Alex
      Anna
      John
      Michael

  ## Exiting the Program
      Command: EXIT
      Ends the program, frees all allocated memory, and stops execution.

      Example: EXIT
      Output: (no message shown)

# Brief presentation of the implementation
This project implements a complete social network simulation using graphs and trees. Below is a detailed explanation of how each feature works:

  ## Friendship Management
  Friendships are stored in adjacency lists, with bidirectional links between users.  
  Adding or removing a friend updates both lists simultaneously.  
  The distance between users is calculated using BFS traversal.  
  Functions like `common`, `popular`, and `suggestions` rely on adjacency traversal and sorting by user ID.

  ## Post and Repost System
  Posts and reposts are represented as nodes in a tree structure.  
  Each node stores:
  - A unique ID
  - Author name
  - Title (NULL for reposts)
  - Like count
  - List of child reposts

  This structure allows hierarchical reposting, efficient traversal, and full deletion of subtrees.

  ## Feed Generation
  The feed merges the posts of a user and all their friends, sorted by recency.  
  Implemented using linked lists and priority-based merging for O(n) complexity relative to total posts.

  ## Clique Detection
  The largest complete friend group is found using the **Bron–Kerbosch algorithm**.  
  The algorithm identifies maximal cliques in the friendship graph, selecting the one that contains the queried user.

  ## Likes and Ratio
  Each post/repost maintains an internal counter for likes.  
  The `ratio` command compares the original post with its reposts and displays the one with higher engagement.  
  In case of equality, the post with the smallest ID is preferred.

  ## Memory Management
  All dynamically allocated data structures (graphs, trees, lists) are properly freed at program termination.  
  Valgrind testing confirms no memory leaks across 50+ automated scenarios.

  ## Modularity and Error Handling
  Each major component (`friends.c`, `posts.c`, `feed.c`, `users.c`) is implemented in a separate source file.  
  The main function reads commands using `fgets` and delegates them to the correct handler.  
  Invalid commands are ignored gracefully, with clear feedback to the user.
  
  ## Exit Function
  The `EXIT` command terminates the main loop, deallocates all memory, and ends the program safely.

  ## Checker
  An automated checker was implemented to validate all functionalities.  
  It includes over **50 tests** verifying:
  - Graph connectivity  
  - Repost tree structure  
  - Feed correctness  
  - Ratio and like behavior  
  - Memory stability under Valgrind

  The checker can be run in GUI or legacy mode:
