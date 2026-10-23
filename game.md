# THE NOMAD: THE LAST ISLAND

## Complete game concept, stage-by-stage gameplay, rules, timers, and winner determination

Your game is an online multiplayer survival game inspired by the tension, strategy, and elimination challenges of Alice in Borderland. Five players enter one match, each representing a different community that needs a new home.

I'll explain the complete game from the moment five ships approach the island until the final winner is declared. I'll use Player A, Player B, Player C, Player D, and Player E throughout the examples so you can understand exactly what happens in every situation.

The goal is to make this detailed enough that you can use it as your main game-development specification.

# 1. The main story of the game

['New World' debuts Aeternum trailer at the Game Awards - News | Amazon Game Studios](https://images.openai.com/static-rsc-4/XqyZR47Z2-1j4kNgxslJ38Tfy9tT0tSLdkIe5Xo6KK_tQOr8FjPeCV1YtxOnVGJo_9PTDiqIE-hsFc2Mqxlh5DpxtHFTdXabPwd3vdJPMXeb04QTjsTGsTBihU0l264fslG4ij6qZXcgv55MUF_s3RYIJ1rxDFO6ro8G4U23wWg?purpose=inline)

[Sea of Thieves Season 20: Custom Seas - CDKeyPrices.com News](https://images.openai.com/static-rsc-4/obpgEM5yKJUJMUR0azphqNPfbuE2QZMtsv8Q4HodnBjkNaxeAlMzSGDWYZDxn29MixEiYibV1fHrluiLL5RljlCzDRbDoO1hyzmmf7Rtjq-cBZBzMjoJBgrk7gcED8Bz01IVQDaSVySfRKN4-hPC5fcBqPMR6nPvZC0GcubVy2Y?purpose=inline)

[9 Yıldır Terk Edilen South Molle Adası Hakkında her şey: Hayalet ada 30 milyon dolara satışa sunuldu | Dünya Haberleri](https://images.openai.com/static-rsc-4/p1YMZ_2c0EH_qIoQCcdrgOAgomx4ifykbsRX53WM-PAY1nwUABLnl-E4ZkcaFvoWPoMMmERmUWWV7LC6izOtQXuEw9IIJ432fW1p60dUlgD5PbptfkNGJl5lncBgpah-HlOhBbBRIZnVt-D2Qlk5S_PDoz3J8ND3oVHmORkW1FY?purpose=inline)

6

The world has suffered a catastrophe. The ocean has become dangerously acidic, and communities can no longer safely depend on their ships for long-term survival.

Five communities discover an island containing food, drinkable water, medicine, shelter, and other resources.

However, the island can support only one community under the rules of the game.

Each community sends one champion to compete for the island.

- Player A represents Community A.
- Player B represents Community B.
- Player C represents Community C.
- Player D represents Community D.
- Player E represents Community E.

All five players enter the same online match. They must pass three challenges: the Number Game, the Hidden Symbol Game, and the Race to the Guardian.

The final two players face the ultimate decision: claim personal victory at the cost of their community, or sacrifice personal glory to save their people.

The central objective is to determine which community gets the chance to settle on the island and which champion ultimately wins the game.

# 2. Complete game flow

Opening — Five Ships Arrive

5 players: A, B, C, D, E

Stage 1 — Number Game

5 players → 1 eliminated → 4 remain

Stage 2 — Hidden Symbol

4 players → 2 eliminated → 2 remain

Stage 3 — Race to the Guardian

2 finalists race to the switch

Final — The Choice

Personal victory or community sacrifice

Ending — Winner and Community Outcome

The game records the champion, the sacrifice ending, or a double-sacrifice ending.

The match begins with five players. Stage 1 removes one player, Stage 2 removes two more, and the final race determines which of the two remaining players gets the first choice.

One distinction is important: winning the race gives a player the first choice; it does not automatically make that player the overall winner.

# 3. Opening sequence — Five ships arrive at the island

[Sea of Remnants プレビュー:GaaS要素にかき消されてしまう可能性は十分にあります プレビュー - Gamereactor](https://images.openai.com/static-rsc-4/J553TigUnITsQUNBCckbB20yXrEY1JpLnaHBYgRgs17iO2LVj8gwjc7WkcQf_Q4Cgtr0vBNr_4KqHyLDwH871e9e7vWwS4l6VHKmWvybMliBDQGfGe_seoG328EscSzqHwYgk6YITs7tblY76Ay8snhxC7kQ709TS46Ka5lh1b8?purpose=inline)

[Lost Rift on Steam](https://images.openai.com/static-rsc-4/Qm2oawKjre2evDmxrYqmq2c-MZX_SU7MB0QcTVF3HRMAZ2fPXupNhLUYkn_MLo-SLw9JJf5_JLp7SjoEuCmx13NIxDD3ZvsTW03P0Cq1ww3ZKMeirZ4NFaAqOaUPCpcmPJqpY4O8fLurfAjeajOVS2thIzljoUAxGPgD7lZWPLU?purpose=inline)

[Sea of Thieves Season 20: Custom Seas - CDKeyPrices.com News](https://images.openai.com/static-rsc-4/obpgEM5yKJUJMUR0azphqNPfbuE2QZMtsv8Q4HodnBjkNaxeAlMzSGDWYZDxn29MixEiYibV1fHrluiLL5RljlCzDRbDoO1hyzmmf7Rtjq-cBZBzMjoJBgrk7gcED8Bz01IVQDaSVySfRKN4-hPC5fcBqPMR6nPvZC0GcubVy2Y?purpose=inline)

7

## Step 1: Five players join the online lobby

Five different people join the same game session.

| Player   | Community   | Ship   |
| -------- | ----------- | ------ |
| Player A | Community A | Ship A |
| Player B | Community B | Ship B |
| Player C | Community C | Ship C |
| Player D | Community D | Ship D |
| Player E | Community E | Ship E |

Each player is assigned a champion character and community identity.

The lobby displays all five players, their connection status, and whether they are ready.

Lobby rules:

- Exactly five players are required to start a standard match.
- The game begins when all five are ready.
- A 15-second countdown begins after everyone is ready.
- If a player disconnects before the match starts, allow 30 seconds for them to reconnect. If they do not return, cancel or restart the lobby.

## Step 2: The arrival cinematic

Recommended timer: 20 seconds, with a skip option for players who have already seen it.

The cinematic shows the five ships travelling across the dangerous ocean. The island appears in the distance, revealing abandoned hotels, forests, ruined buildings, and the guardian's location.

The ships arrive at the island, and the five champions disembark.

A message appears:

> Five communities have found their last hope. Only one community can claim this island. Your survival depends on your choices.

The players then move to the first challenge arena.

## Step 3: The first challenge briefing

Recommended timer: 10 seconds.

The screen explains the rules of the Number Game. Every player can see the rules and their starting lives.

Each player begins Stage 1 with three lives.

No player can be eliminated during the arrival cinematic or briefing. Elimination begins only when the first challenge starts.

# 4. Stage 1 — The Number Game

[The Traitors: Live Experience | Official Tickets](https://images.openai.com/static-rsc-4/IIitekttn-9EUtF__7p6BhqQZE5erVpzF9Ixg5BljaFXe_SFgd8RlknCJE-m33n0ilMg2XSoVzVqHmdsXjg-jyqRR77twSE-RC5ANmm7rONyNduffmU89xP8I5Jz8wsPDliXCdjkeXGAzKp4FzXbCP3Gep7zx4VUShGL3R3BJsU?purpose=inline)

[Elm Craft Games — Developer Studio](https://images.openai.com/static-rsc-4/AqLxnvsrD9HPUAwAhvAmqns0c_-2vX5lOZNhvolLPpuVG5RiVFeHX66XA9WNa08emLRCxPGQ3xVqKmFl2413yg35W9Z-t5_xfXNtYertc80WvuY-zMP_b0W29PhyRF7q5dEKiOgsD2RlcyIGe7wUGUPK8Fzb2SUSe5UPAtnv97M?purpose=inline)

[Как снимают шоу «Игра вслепую» на ТНТ: когда и как проходят съемки, интересные факты о проекте](https://images.openai.com/static-rsc-4/eS9XdrEfH3fsyldfu9lviC1J9riaL-OZEjeYRNgPtn6Jf-UGRcdVt2CP5tYjwIqMWuJ_8tDl8iqR7k9gY3FfW5zTbrI6q8CjS1fdFqzCXM5OdZy8r8tAKgvDkoBUuYuMT3eTk_kVFiBNBUVE8ImS1wDppn31fNiPr14Xn2uq74o?purpose=inline)

7

## Stage 1 settings

5 players

Starting lives per player

3

Number range

0–100

Number selection timer

10 seconds

Players advancing

4

## 4.1 How the game begins

Players A, B, C, D, and E enter a dark room and sit around a virtual table.

Every player sees a number selector ranging from 0 to 100. Each player must secretly choose one number.

The goal is to predict the number that other players will collectively choose.

The server calculates the average of all five submitted numbers, then multiplies the average by 0.8 (80%) to produce the target.

The player whose number is closest to the target wins that round and keeps their lives. Every other player loses one life.

## 4.2 Exact timer for each round

| Action                          | Timer      |
| ------------------------------- | ---------- |
| Round introduction              | 5 seconds  |
| Choose and submit a number      | 10 seconds |
| Calculate and reveal the target | 3 seconds  |
| Show results and update lives   | 5 seconds  |

The total is approximately 23 seconds per round, excluding loading delays.

Important: The server must collect all valid submissions before calculating the average. Players must not see other players' choices before the round resolves.

If someone fails to submit a number in time, the server assigns a random number from 0 to 100. The rule must be shown to everyone before the match starts.

## 4.3 Example: Round 1

Suppose all five players submit these numbers:

| Player | Selected number | Starting lives |
| ------ | --------------- | -------------- |
| A      | 10              | 3              |
| B      | 20              | 3              |
| C      | 30              | 3              |
| D      | 40              | 3              |
| E      | 50              | 3              |

The average is:

\\[ \frac{10+20+30+40+50}{5}=30 \\]

The target is 80% of the average:

\\[ 30\times0.8=24 \\]

Player B selected 20, which is closest to the target of 24.

Therefore:

- Player B wins the round and keeps three lives.
- Players A, C, D, and E each lose one life.

The updated life count becomes:

| Player | Lives after Round 1 |
| ------ | ------------------- |
| A      | 2                   |
| B      | 3                   |
| C      | 2                   |
| D      | 2                   |
| E      | 2                   |

No player is eliminated yet.

## 4.4 Example: Round 2

The four players repeat the game with the same number range and timer.

Suppose the selections are:

| Player | Selected number | Lives before round |
| ------ | --------------- | ------------------ |
| A      | 10              | 2                  |
| B      | 50              | 3                  |
| C      | 20              | 2                  |
| D      | 30              | 2                  |
| E      | 40              | 2                  |

The average is 30, and the target is 24.

Player C, who selected 20, is closest to the target. C wins the round and keeps two lives.

The other players lose one life:

| Player | Lives after Round 2 |
| ------ | ------------------- |
| A      | 1                   |
| B      | 2                   |
| C      | 2                   |
| D      | 1                   |
| E      | 1                   |

Again, no one is eliminated.

## 4.5 Example: Round 3

Suppose the players submit:

| Player | Selected number | Lives before round |
| ------ | --------------- | ------------------ |
| A      | 5               | 1                  |
| B      | 40              | 2                  |
| C      | 20              | 2                  |
| D      | 25              | 1                  |
| E      | 30              | 1                  |

The average is 24, and the target is 19.2.

Player C's selection of 20 is closest to the target.

C keeps two lives. Every other player loses one life.

The results are:

| Player | Lives after Round 3 | Status     |
| ------ | ------------------- | ---------- |
| A      | 0                   | Eliminated |
| B      | 1                   | Active     |
| C      | 2                   | Active     |
| D      | 0                   | Eliminated |
| E      | 0                   | Eliminated |

This example reveals an important rule issue: the round eliminated three players, but Stage 1 must eliminate only one player so that four advance.

Therefore, you should not use a simple “every loser loses one life” rule without a safeguard.

## 4.6 Recommended elimination rule for Stage 1

For a predictable five-to-four progression, use this rule instead:

1. Everyone selects a number.
2. The target is calculated.
3. The player closest to the target wins the round.
4. The player farthest from the target loses the round.
5. If two or more players tie for the farthest distance, those tied players enter a sudden-death round.
6. The losing player is eliminated, and the other four advance.

You can still display lives as a secondary score or use them in a longer tournament mode. But for your first hackathon prototype, one loser per round is simpler, easier to test, and guarantees that Stage 1 ends with four players.

### Final example for Stage 1

Suppose the five players choose 10, 20, 30, 40, and 50. The target is 24.

- B is closest to the target and wins the round.
- E is farthest from the target and loses.
- If no tie-break is required, E is eliminated.

The advancing players are A, B, C, and D.

The game displays:

> Player E has been eliminated. Four champions remain.

Player E enters spectator mode and can watch the rest of the match. E cannot influence the remaining challenges.

This is the recommended final version of Stage 1.

# 5. Stage 2 — The Hidden Symbol Game

[Эти названия в дораме звучали не просто так: что значат масти карт в сериале «Алиса в Пограничье»](https://images.openai.com/static-rsc-4/PcBE1Ow7ciC2iO5vBkSbO7xdNTMGujz37kQNpioz2QxMS-p6_mvd0mfCexGJlMq3SXrFMlIhevWRVgqdNW5ixwTnm-p8aiiMUK4SQ3ixVKuMhPpHhce3BHy9Ec1BUg23esnOSjVRvdZANyYeckJG9lclUUOHUBYEkqSDSdVN_PQ?purpose=inline)

[Aces cards symbols with futuristic neon blue and red lights on a black background](https://images.openai.com/static-rsc-4/fIEcg_MmVOHniyM15ug9fT-iDsZcBMInjInPteCRUvU9138DdJtaX0j-EbGG5nyH3wh7kdYeitGUeAP09aqNOCz2O6uxz5TRK0qjxT9e67hBTVXU9KjVudyNIzHRHKbBll7WPmoHXityRpYTVEHfQD5iYk5Cqd_u7klromYBM4B_HmWvp64tyZqsHnMM3Ace?purpose=inline)

[「今際の国のアリス あらすじまとめ｜ドラマ1話～最終話まで解説」 | momoiroblog](https://images.openai.com/static-rsc-4/ZJ-gGUqoeeB1xq7qAionKejpTZxUcVFC5_LPUyef3kGJAy19H0lasL3iHzSzOJSpVsvFgKbC-0BwyAny3s5m6lODrVWONrAsTWghx4zdWLBs6dfnlVm2WbWH7ZGv-xp3qnb-bWeELVoWiCSwgEO9CutMhs34-JqTenxlqUtVA7E?purpose=inline)

8

## Stage 2 settings

4 players

Symbols

Heart, Diamond, Club, Spade

Communication timer

30 seconds

Guess submission

10 seconds

Players advancing

2

## 5.1 How the stage begins

After Player E is eliminated, Players A, B, C, and D enter an abandoned hotel.

They see four symbols:

- ♥ Heart
- ♦ Diamond
- ♣ Club
- ♠ Spade

The server secretly assigns one symbol to each player.

For example:

| Player | Own symbol | What the player can see   |
| ------ | ---------- | ------------------------- |
| A      | Heart      | B's, C's, and D's symbols |
| B      | Spade      | A's, C's, and D's symbols |
| C      | Diamond    | A's, B's, and D's symbols |
| D      | Club       | A's, B's, and C's symbols |

These symbols are an example of one assignment, not information players should automatically receive about themselves.

Each player can see the symbols of the other three players, but their own symbol is hidden from their screen.

The objective is to discover their own symbol through communication and deduction.

## 5.2 Exact timer

| Action            | Timer      |
| ----------------- | ---------- |
| Stage briefing    | 8 seconds  |
| Communication     | 30 seconds |
| Submit your guess | 10 seconds |
| Reveal results    | 5 seconds  |

A complete round takes approximately 53 seconds.

During the 30-second communication period, players can use in-game text chat to tell each other what they see.

For example, Player A can see that Player B has a Spade. A may tell B, “Your symbol is a Spade.”

But the game should allow players to lie. A player might tell another player the wrong symbol to mislead them.

After communication ends, everyone submits their guess privately.

## 5.3 Example: First round

Assume the server assigned these symbols:

- A = Heart
- B = Spade
- C = Diamond
- D = Club

During the communication period:

- A sees B's Spade, C's Diamond, and D's Club.
- B sees A's Heart, C's Diamond, and D's Club.
- C sees A's Heart, B's Spade, and D's Club.
- D sees A's Heart, B's Spade, and C's Diamond.

Players communicate, share information, and decide what to believe.

Suppose they submit these guesses:

| Player | Actual symbol | Guess   | Result    |
| ------ | ------------- | ------- | --------- |
| A      | Heart         | Heart   | Correct   |
| B      | Spade         | Heart   | Incorrect |
| C      | Diamond       | Diamond | Correct   |
| D      | Club          | Spade   | Incorrect |

A and C answer correctly. B and D answer incorrectly.

However, if incorrect guesses immediately eliminate players, both B and D would be eliminated at the same time. You need a rule to control this.

## 5.4 Recommended elimination rule

To make the stage end with exactly two finalists:

1. Players submit their guesses.
2. Correct guesses earn one qualification point.
3. Incorrect guesses receive no point.
4. Repeat the round with a fresh symbol assignment.
5. After the required number of rounds, rank the players by qualification points.
6. The top two players advance.
7. If players tie for the second qualifying position, use a sudden-death symbol round among the tied players.

For a short prototype, use three symbol rounds, each with the same timers. The top two players by correct answers advance.

In the example above, A and C begin with one correct answer, while B and D have zero. More rounds are played before the final ranking.

If there is still a tie, a sudden-death round determines who advances.

This avoids the problem of accidentally eliminating all four players at once.

### Final result of Stage 2

Suppose the final scores are:

| Player | Correct guesses | Final status |
| ------ | --------------- | ------------ |
| A      | 3               | Advances     |
| B      | 1               | Eliminated   |
| C      | 2               | Advances     |
| D      | 1               | Eliminated   |

Players A and C become the finalists.

The game displays:

> Two champions remain. The race to the Guardian begins.

Important implementation detail: The server must never send a player's own hidden symbol to that player's client. Otherwise, a player could inspect their network data and cheat.

# 6. Stage 3 — The Race to the Guardian

[The Crew Motorfest Cross-Gen Bundle](https://images.openai.com/static-rsc-4/oS-WrcT_Y99ZhVh2ByfyPUKfvrSugc70-w0YLfY7EzgPYLeWV67yAsEuwhXem-j2SBKxmAGAtRZ6DwkHpUPX08G4B6ANJ6RmCQ6nuTv4Y1ZS2_ucXxv2SIWp1AS5cj7l8OISIXmVp5dOhAwpsPRH1RTiZxeZXuPaG-XdfA6lq_8?purpose=inline)

[Forza Horizon 5 - Creating the Next-Generation of Open-World Racers - AusGamers.com](https://images.openai.com/static-rsc-4/CkJH3IyQ6RcT7x-dN1L2kHsrcBqO7bAanDr4sJ--b3wBTjh6l_pHVvoe_rTBTKavgEfOWX818zkB_AMwYXuaA2tKSqT60CnsGry-DrjJCWxt7550PsM0hFHR_BJzrdjTnUOReHMZ7FB-NLIoRwksisHoUCVTbW-LgcSY8YUxsBg?purpose=inline)

[Most Loved 3D Kits from the KitBash3D Community](https://images.openai.com/static-rsc-4/6RKtIlAlQYplPh5yTwRLUcqlpxW26ivaQ-FSLDQcbCOsrR7kqt1Jlhs3C7VFUxFMcN320ER7oMQONRcqwdNM0SjSgtm9GtIzgbnaVZsJtSGpRsav1IVCAgy1HGwr0R9_eVKduoHnwxU5HZkn-BQ3wD27bIC5EfcHzcL7QTvuGaw?purpose=inline)

8

This stage is different from the previous challenges. It is a race rather than a prediction or deduction game.

Players A and C are the two finalists. Both receive a vehicle and must reach the Guardian's location.

## 6.1 The starting area

Both players appear at separate starting positions.

Each vehicle has identical:

- Maximum speed
- Acceleration
- Braking
- Handling
- Starting fuel, if fuel is implemented

The guardian's location appears on the map. Both players can see a waypoint showing the destination.

## 6.2 Exact timer

| Action             | Timer      |
| ------------------ | ---------- |
| Race briefing      | 10 seconds |
| Starting countdown | 3 seconds  |
| Race duration      | 90 seconds |
| Switch interaction | 3 seconds  |

When the countdown reaches zero, both vehicles start moving.

## 6.3 How the race works

Player A and Player C drive through the island toward the guardian.

The island route contains obstacles, sharp turns, and alternative paths. Players must drive carefully while trying to reach the destination first.

The first player to enter the switch's interaction area and activate it successfully receives First Choice status.

For example:

- Player A reaches the guardian after 42 seconds.
- Player C reaches the guardian after 47 seconds.
- A activates the switch first.
- A receives the first opportunity to make the final choice.

However, A has not automatically won the game.

The race determines the order of the final decision, not the final champion.

## 6.4 What if neither player reaches the guardian?

If neither player reaches the guardian within 90 seconds, the game enters a sudden-death race.

A simple rule for the prototype:

- Both players return to the starting area.
- The route becomes shorter.
- Both players receive identical vehicles.
- A new 45-second timer starts.
- The first player to activate the switch receives First Choice status.

If one player disconnects during the race, allow 20 seconds to reconnect. If they do not return, the other player wins by forfeit.

## 6.5 What happens after the switch?

The game transitions to the final chamber. The players receive a briefing explaining the two final options.

The first player to activate the switch gets to choose first.

If that player chooses personal victory, the match ends immediately. If they choose sacrifice, the other finalist receives the opportunity to make their own choice.

This leads to the most important part of your game.

# 7. The Final Stage — The Guardian's Choice

[From Consequences to Trust: Applying the System Fairly - GameDev Platform | Boosty](https://images.openai.com/static-rsc-4/g_j5YjT122N-FHF-kki_aGEEsJQtzqCNdkmNsa_mhCIkbUAX1FSdrYjONTUDp1rQMZUbEPSh_Bvxt4PvXm8dBFJ3f4IDt-GMuJTfmIG0lhDL5lySmSbCm23rB5A7XBqjcLMCGkawYk7ZmvUhRMtV4lF84zmeRFXD6uFZyse4GTs?purpose=inline)

[Swords of Legends Announced for PC and Consoles - GamerBraves](https://images.openai.com/static-rsc-4/fvrD91gHafJhF9Dr8-6tF50xRayA5AVK4AWOESGEwGIf8_Vw0B2jjMwpejSSYPXpGZeWiiQTPdRcwYNnQELhyAda6BBFDXqZdAwWJkO_GDaoGgm8RDDh-kI1ZYUu-rG4k6l1eM70ZxoJCOFciciVY49bJFS07fXgciIiUCFRp4M?purpose=inline)

[Download Ancient Throne Room Remnant 2 Wallpaper | Wallpapers.com](https://images.openai.com/static-rsc-4/Xq3Y_oCQ3YVTb70pLip3pp73qYKReGMAbSdqIMOmPhkjkX3UpBOxSfschpuJJMAbA_UMjdxmqeG50-LCzdVEB0I6fqlHxVFv_uE3nlP9gAuLsXpZzg_uSvenKXdB0W1iR8rr_G6P11gWHp9cPdRoI13kR8dYlPiK-q1F-3jVs08?purpose=inline)

8

This is the climax of your game. It should feel different from every other stage because the player is no longer simply trying to score points or finish a race.

They must decide what matters more: their own victory or the survival of their community.

## 7.1 The two options

When Player A activates the switch first, two options appear on the screen.

## Option 1 — Become the King

Personal victory

- The champion survives.
- The champion becomes the island's ruler.
- Their community does not survive.
- Their name appears on the leaderboard as the winner.
- The champion's community is not allowed to settle on the island.

Result: The player wins personally but loses their people.

## Option 2 — Save Your People

Personal sacrifice

- The champion sacrifices their character's life.
- The champion does not receive the individual champion title.
- Their community is saved.
- The champion gives up their chance to claim the island.
- The next finalist receives their opportunity to choose.

Result: The player loses personally but saves their people.

## 7.2 Final-choice timer

| Action            | Timer         |
| ----------------- | ------------- |
| Final briefing    | 10 seconds    |
| Select an option  | 15 seconds    |
| Confirm selection | 5 seconds     |
| Ending cinematic  | 15–20 seconds |

The 15-second selection timer begins when the two options become available.

When a player selects an option, a confirmation screen appears. The player has five seconds to confirm.

Once confirmed, the choice cannot be reversed.

If a player does not choose before the timer expires, the game automatically selects Become the King. This default must be clearly explained before the match so nobody is surprised by an automatic decision.

## 7.3 Example: Player A reaches the switch first

At the beginning of the final stage:

- Player A represents Community A.
- Player C represents Community C.
- A reached the guardian first.
- A receives the first choice.

A must decide whether to become the king or sacrifice themselves.

### Scenario A: Player A chooses Become the King

A selects Option 1 and confirms.

The result is:

- A survives.
- A becomes the island's ruler.
- Community A does not survive.
- Community C does not receive the island.
- The game ends immediately.
- A is recorded as the official champion.

The screen displays:

> PLAYER A HAS CLAIMED THE ISLAND.

The ending cinematic shows A standing alone on the island, surrounded by its resources. The victory screen records A's name, but the fate of Community A makes the victory bittersweet.

### Scenario B: Player A chooses Save Your People

A selects Option 2 and confirms.

The result is:

- A sacrifices their character's life.
- Community A is rescued.
- A does not become the island's champion.
- Player C receives the next choice.

Now C must decide whether to claim the island or sacrifice themselves.

But this situation needs one clear rule: if A's community is automatically awarded the island, C cannot also independently claim that same island.

To preserve your original idea that C gets a choice, I recommend the following consistent rule.

- A's sacrifice guarantees that Community A survives, but it does not automatically grant ownership of the island.
- Community A receives an emergency evacuation route to a safe location.
- Player C can still claim the island for Community C.
- If C also sacrifices, both communities are rescued, but neither claims the island.

This keeps A's sacrifice meaningful without giving two communities exclusive ownership of the same island.

### Scenario C: Player C chooses Become the King

After A sacrifices, C receives the choice.

C selects Option 1.

The result is:

- C survives.
- C becomes the island's ruler.
- Community C claims the island's resources.
- Community A survives through the evacuation route.
- C becomes the official champion.

The leaderboard records C as the winner and A as the player who sacrificed themselves.

### Scenario D: Both players sacrifice themselves

A sacrifices first, saving Community A. C then also selects the sacrifice option.

The result is:

- Both champions sacrifice their characters' lives.
- Both communities are rescued.
- Neither community claims the island.
- The island remains unclaimed.

The game records a special Double Sacrifice Ending rather than declaring an individual champion.

This is a unique alternative ending to the normal victory condition. It gives the players a meaningful choice instead of forcing a winner when both finalists reject personal victory.

# 8. How the winner is determined

This is the complete winner-determination logic for your game.

| Final event                                               | Official outcome                                                   |
| --------------------------------------------------------- | ------------------------------------------------------------------ |
| First finalist chooses Become the King                    | That finalist wins the island immediately                          |
| First finalist sacrifices; second chooses Become the King | Second finalist wins the island                                    |
| Both finalists sacrifice                                  | Special ending; both communities survive, island remains unclaimed |
| A finalist disconnects during the final                   | Apply the reconnect and forfeit rules                              |
| A player is eliminated in Stage 1 or 2                    | They cannot return to active competition                           |

The official leaderboard should distinguish between the champion, sacrificed champions, and eliminated players.

For example, if A sacrifices and C becomes the king, the leaderboard could display:

- Champion: Player C
- Finalist who sacrificed: Player A
- Eliminated in Stage 2: Players B and D
- Eliminated in Stage 1: Player E

The leaderboard should never falsely show a player as the champion simply because they reached the final or activated the switch first.

# 9. Complete timer reference

Here is the full timing plan for the initial prototype.

| Game phase                           | Timer         |
| ------------------------------------ | ------------- |
| Lobby countdown                      | 15 seconds    |
| Arrival cinematic                    | 20 seconds    |
| First-stage briefing                 | 10 seconds    |
| Number Game round introduction       | 5 seconds     |
| Number selection                     | 10 seconds    |
| Number result calculation and reveal | 3 seconds     |
| Number results display               | 5 seconds     |
| Hidden Symbol briefing               | 8 seconds     |
| Hidden Symbol communication          | 30 seconds    |
| Hidden Symbol guess submission       | 10 seconds    |
| Hidden Symbol results display        | 5 seconds     |
| Race briefing                        | 10 seconds    |
| Race countdown                       | 3 seconds     |
| Race to the Guardian                 | 90 seconds    |
| Guardian switch interaction          | 3 seconds     |
| Final briefing                       | 10 seconds    |
| Final option selection               | 15 seconds    |
| Final choice confirmation            | 5 seconds     |
| Ending cinematic                     | 15–20 seconds |

These are starting values. You should test them with actual players and adjust the timing if challenges feel too rushed or too slow.

# 10. Important multiplayer rules

Because your game is online, the server—not the player's browser or game client—must control the important decisions.

- The server decides when each timer starts and ends.
- The server calculates the Number Game target.
- The server determines who advances and who is eliminated.
- The server privately assigns the hidden symbols.
- The server validates guesses and updates qualification scores.
- The server determines who activates the guardian switch first.
- The server records the final choice and determines the ending.

Eliminated players can spectate, but they cannot interact with the active challenge or influence its result.

If a player disconnects, the game allows a 20-second reconnect period during active stages. If they do not return, the match applies the documented forfeit rule instead of remaining stuck indefinitely.

# 11. The complete example: From five players to the ending

Let's walk through one complete match.

Beginning

A, B, C, D, and E arrive on the island aboard five ships. Each represents a different community.

Stage 1

The five players compete in the Number Game. E finishes farthest from the target and is eliminated. A, B, C, and D advance.

Stage 2

The four players compete in the Hidden Symbol Game. After the required rounds, A and C qualify. B and D are eliminated.

Stage 3

A and C race to the Guardian. A arrives first and receives the first choice.

Final

A chooses to sacrifice themselves, saving Community A. C then chooses to become the king, claiming the island for Community C.

Ending

C is the official champion. Community C settles on the island, and Community A survives through the evacuation route. A receives recognition for the sacrifice ending, but not the champion title.

# 12. What you should build first

Do not try to build the entire island, all challenges, the race, and the final ending simultaneously.

Build in this order:

1. Five-player online lobby and ship-arrival scene.
2. Number Game with a 10-second submission timer and one-player elimination.
3. Hidden Symbol Game with private symbol assignments, chat, and two-player qualification.
4. Two-vehicle race with a server-controlled finish and switch.
5. Final decision, community outcomes, and ending screens.
6. Spectator mode, leaderboard, reconnect handling, and visual polish.

Once these pieces work together, you will have a playable end-to-end prototype.

The most important rule to finalize before coding the ending is the sacrifice outcome: in the recommended version, sacrificing guarantees the community survives, but does not automatically give it ownership of the island. This allows the second finalist to make a meaningful choice while keeping the island's ownership rules consistent.