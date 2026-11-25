/*
 * Predicted weapon sounds, and suppression of the server's echo of them.
 *
 * When we predict a shot we play its sound immediately. The server plays the
 * same sound a round trip later, and we have to drop that copy or the player
 * hears everything twice.
 *
 * We do that with one-shot tokens rather than a permanent "always suppress
 * these sounds" list: firing pushes a token, and the first matching server
 * sound consumes it. A sound we did not predict, because prediction was off or
 * disagreed with the server, still reaches the player. Tokens expire so a
 * prediction the server never confirms cannot silence a later real sound.
 */

typedef struct
{
	string	sample;
	float	chan;
	float	expires;	// a token is live while this is still ahead of cltime
} token_t;

static token_t tokens[MAX_SOUND_TOKENS];

void Sound_Reset(void)
{
	// Expiring a token is all it takes to retire it; nothing reads the rest.
	for (float i = 0; i < tokens.length; i++)
		tokens[i].expires = 0;
}

static void Sound_PushToken(string sample, float chan)
{
	float oldest = 0;

	for (float i = 0; i < tokens.length; i++)
	{
		if (tokens[i].expires <= cltime)
		{
			oldest = i;
			break;
		}

		// All slots live: overwrite whichever expires first.
		if (tokens[i].expires < tokens[oldest].expires)
			oldest = i;
	}

	tokens[oldest].sample = sample;
	tokens[oldest].chan = chan;
	tokens[oldest].expires = cltime + SOUND_TOKEN_TTL;
}

// Play a sound we are predicting, and arrange for the server's copy to be
// dropped when it arrives.
void Sound_PlayPredicted(string sample, float chan)
{
	if (sample == "")
		return;

	if (!Pred_SoundEnabled())
		return;

	Sound_PushToken(sample, chan);

	// ATTN_NONE: this is our own weapon, it should not fall off with distance.
	sound(viewweapon, chan, sample, 1, ATTN_NONE);
}

/*
 * Take the token matching a sound the server is about to play, if we hold one.
 * Returns true when the caller should drop that sound as our own echo.
 */
float Sound_Consume(string soundname, float channel)
{
	for (float i = 0; i < tokens.length; i++)
	{
		if (tokens[i].expires <= cltime)
			continue;
		if (tokens[i].sample != soundname)
			continue;

		// A token on CHAN_AUTO was played on a channel of the engine's choosing,
		// so accept the sound whatever channel it arrives on.
		if (tokens[i].chan != channel && tokens[i].chan != CHAN_AUTO)
			continue;

		tokens[i].expires = 0;
		return TRUE;
	}

	return FALSE;
}
