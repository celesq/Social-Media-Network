# Social-Media-Network
Developed a social media platform using data structures for user connections and posts. It includes features like adding/removing friends, distance between users, and post/repost management using a tree structure. The feed shows friends' posts, with advanced functionalities like "friends that reposted" and clique detection.

# User Guide

  ## Add Friend
        Command: add <name-1> <name-2>
        Creates a connection between two users, adding them as friends.
        Example: add John Alice
        Output: Added connection John - Alice

  ## Remove Friend
        Command: remove <name-1> <name-2>
        Removes the friendship between two users.
        Example: remove John Alice
        Output: Removed connection John - Alice

  ## Distance Between Users
        Command: distance <name-1> <name-2>
        Calculates the number of connections between two users.
        Example: distance John Alice
        Output: The distance between John - Alice is 2

  ## Friend Suggestions
        Command: suggestions <name>
        Lists friends of friends who are not yet connected to the user.
        Example: suggestions Alice
        Output: Suggestions for Alice: Bob, Claire

  ## Common Friends
        Command: common <name-1> <name-2>
        Displays mutual friends between two users.
        Example: common John Alice
        Output: The common friends between John and Alice are: Bob

  ## Number of Friends
        Command: friends <name>
        Shows how many friends a user has.
        Example: friends Alice
        Output: Alice has 3 friends

  ## Most Popular Friend
        Command: popular <name>
        Displays the most connected user among a user’s friends.
        Example: popular Alice
        Output: Bob is the most popular friend of Alice

# Posts and Reposts

  ## Create Post
        Command: create <name> <title>
        Creates a new post for the user with a unique ID.
        Example: create John "My first post"
        Output: Created "My first post" for John

  ## Repost
        Command: repost <name> <post-id> [repost-id]
        Creates a repost of an existing post or another repost.
        Example: repost Alice 1
        Output: Created Repost #5 for Alice

  ## First Common Repost
        Command: common-repost <post> <repost-id-1> <repost-id-2>
        Finds the first common repost between two reposts.
        Example: common-repost 1 4 7
        Output: The first common repost of 4 and 7 is 2

  ## Like
        Command: like <name> <post-id> [repost-id]
        Adds or removes a like on a post or repost.
        Example: like Alice 1
        Output: Alice liked "Post Title"

  ## Ratio
        Command: ratio <post-id>
        Checks if any repost has more likes than the original post.
        Example: ratio 1
        Output: Post 1 got ratio'd by repost 2

  ## Delete Post/Repost
        Command: delete <post-id> [repost-id]
        Deletes a post or a repost along with its child reposts.
        Example: delete 1
        Output: Deleted "Post Title"

  ## Get Likes
        Command: get-likes <post-id> [repost-id]
        Displays the number of likes on a post or repost.
        Example: get-likes 1
        Output: Post "Post Title" has 3 likes

  ## Get Reposts
        Command: get-reposts <post-id> [repost-id]
        Displays the hierarchy of reposts for a post.
        Example: get-reposts 1
        Output: Lists all reposts and users who made them.

# Social Feed and Profile

  ## Feed
        Command: feed <name> <feed-size>
        Shows recent posts from a user and their friends.
        Example: feed John 5
        Output: Displays the 5 most recent posts.

  ## View Profile
        Command: view-profile <name>
        Displays all posts and reposts by a user.
        Example: view-profile Alice
        Output: Lists all posts and reposts by Alice.

  ## Friends That Reposted
        Command: friends-repost <name> <post-id>
        Lists friends who reposted a specific post.
        Example: friends-repost John 1
        Output: Lists all friends who reposted post 1.

  ## Clique Detection
        Command: common-group <name>
        Finds the largest group of mutual friends that includes the user.
        Example: common-group John
        Output: Lists all users in John's largest friend group.

# Brief explanation of the implementation

 ## Social Network

  Add Friend: Established a connection between two users using the add_edge function.
  Remove Friend: Removed the connection between two users with remove_edge.
  Find Distance: Used BFS to calculate the shortest path between users.
  Suggestions: Modified BFS to run for two iterations, tracking visited users and avoiding direct friends.
  Common Friends: Checked mutual friends by comparing the adjacency lists of two users.
  Number of Friends: Displayed the number of nodes in a user’s adjacency list.
  Most Popular: Found the user with the most direct friends by comparing adjacency list sizes.

 ## Posts and Reposts

  ### Data Structures: Created a post_t structure for posts/reposts and a tree_t structure for hierarchical relationships. Used an aprecieri structure to store likes.
  ### Create Post: Added a new post_t entry with relevant fields (name, UID, ID).
  ### Repost: Similar to post creation, but the title is NULL, and the parent ID is linked to the original post.
  ### First Common Repost: Traversed both reposts' paths to the root to find the first common node.
  ### Like: Managed likes by adding/removing users from the likes list and adjusting like counts accordingly.
  ### Ratio: Checked if any repost had more likes than the original post and displayed the result.
  ### Delete Post/Repost: Used a recursive delete_repost function to delete all children of a post/repost.
  ### Get Likes: Displayed the number of likes for a post or repost.
  ### Get Reposts: Displayed the full repost hierarchy of a post.

## Social Feed

  ### Feed: Iterated through posts/reposts from end to start and checked if they belonged to friends.
  ### View Profile: Displayed all posts and reposts made by a user.
  ### Friends That Reposted: Searched the post’s tree to see if any friends reposted it.
  ### Clique Detection: Implemented the Bron-Kerbosch algorithm to find and display the largest friend clique.
