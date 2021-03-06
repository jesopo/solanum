/*
 *   IRC - Internet Relay Chat, contrib/m_findforwards.c
 *   Copyright (C) 2002 Hybrid Development Team
 *   Copyright (C) 2004 ircd-ratbox Development Team
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "stdinc.h"
#include "channel.h"
#include "client.h"
#include "hash.h"
#include "match.h"
#include "ircd.h"
#include "numeric.h"
#include "s_user.h"
#include "s_conf.h"
#include "s_newconf.h"
#include "send.h"
#include "msg.h"
#include "parse.h"
#include "modules.h"
#include "packet.h"
#include "messages.h"

static const char findfowards_desc[] = "Allows operators to find forwards to a given channel";

static void m_findforwards(struct MsgBuf *msgbuf_p, struct Client *client_p, struct Client *source_p,
			int parc, const char *parv[]);

struct Message findforwards_msgtab = {
	"FINDFORWARDS", 0, 0, 0, 0,
	{mg_unreg, {m_findforwards, 2}, mg_ignore, mg_ignore, mg_ignore, {m_findforwards, 2}}
};

mapi_clist_av1 findforwards_clist[] = { &findforwards_msgtab, NULL };

DECLARE_MODULE_AV2(findforwards, NULL, NULL, findforwards_clist, NULL, NULL, NULL, NULL, findfowards_desc);

/*
** mo_findforwards
**      parv[1] = channel
*/
static void
m_findforwards(struct MsgBuf *msgbuf_p, struct Client *client_p, struct Client *source_p, int parc, const char *parv[])
{
	static time_t last_used = 0;
	struct Channel *chptr;
	struct membership *msptr;
	rb_dlink_node *ptr;

	/* Allow ircops to search for forwards to nonexistent channels */
	if(!IsOperGeneral(source_p))
	{
		if((chptr = find_channel(parv[1])) == NULL || (msptr = find_channel_membership(chptr, source_p)) == NULL)
		{
			sendto_one_numeric(source_p, ERR_NOTONCHANNEL,
					form_str(ERR_NOTONCHANNEL), parv[1]);
			return;
		}

		if(!is_chanop(msptr))
		{
			sendto_one(source_p, form_str(ERR_CHANOPRIVSNEEDED),
					me.name, source_p->name, parv[1]);
			return;
		}

		if((last_used + ConfigFileEntry.pace_wait) > rb_current_time())
		{
			sendto_one(source_p, form_str(RPL_LOAD2HI),
					me.name, source_p->name, "FINDFORWARDS");
			return;
		}
		else
			last_used = rb_current_time();
	}

	send_multiline_init(source_p, " ", ":%s NOTICE %s :Forwards for %s: ",
			me.name,
			source_p->name,
			parv[1]);

	RB_DLINK_FOREACH(ptr, global_channel_list.head)
	{
		chptr = ptr->data;
		if(!irccmp(chptr->mode.forward, parv[1]))
		{
			send_multiline_item(source_p, "%s", chptr->chname);
		}
	}

	send_multiline_fini(source_p, NULL);
}
