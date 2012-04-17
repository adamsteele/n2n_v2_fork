/*
 * sn_multiple.c
 *
 *  Created on: Mar 25, 2012
 *      Author: Costin Lupu
 */

#include "n2n.h"
#include "sn_multiple.h"

/*
 * Operations on sn_info lists.
 */

int read_sn_list_from_file( const char *filename, struct sn_info **list )
{
    struct sn_info *sni = NULL;

    FILE *fd = fopen(filename, "r");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open supernodes file");
        return -1;
    }

    unsigned int sn_num = 0;
    if (fread(&sn_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't read supernodes number from file");
        goto out_err;
    }

    int i;
    for (i = 0; i < sn_num; i++)
    {
        sni = calloc(1, sizeof(struct sn_info));
        if (!sni)
        {
            traceEvent(TRACE_ERROR, "couldn't allocate a new supernode entry");
            goto out_err;
        }

        if (fread(&sni->sn, sizeof(n2n_sock_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't read supernode entry from file");
            goto out_err;
        }

        sn_list_add(list, sni);
    }

    sn_reverse_list(list);

    fclose(fd);
    return 0;

    out_err: clear_sn_list(list);
    fclose(fd);
    return -1;
}

int write_sn_list_to_file( const char *filename, struct sn_info *list )
{
    FILE *fd = fopen(filename, "w");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open supernodes file");
        return -1;
    }

    unsigned int sn_num = sn_list_size(list);
    if (fwrite(&sn_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't write supernodes number to file");
        goto out_err;
    }

    while (list)
    {
        if (fwrite(&list->sn, sizeof(n2n_sock_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't write supernode entry to file");
            goto out_err;
        }

        list = list->next;
    }

    fclose(fd);
    return 0;

    out_err: fclose(fd);
    return -1;
}

void sn_list_add( struct sn_info **list, struct sn_info *new )
{
    new->next = *list;
    *list = new;
}

size_t clear_sn_list( struct sn_info **sn_list )
{
    size_t retval = 0;
    struct sn_info *scan = *sn_list;

    while (scan)
    {
        struct sn_info *crt = scan;
        scan = scan->next;
        free(crt);
        retval++;
    }
    *sn_list = NULL;

    return retval;
}

size_t sn_list_size( const struct sn_info *list )
{
    size_t retval = 0;
    while (list)
    {
        ++retval;
        list = list->next;
    }
    return retval;
}

void sn_reverse_list( struct sn_info **list )
{
    struct sn_info *aux = *list, *next = NULL;
    *list = NULL;

    while (aux)
    {
        next = aux->next;
        sn_list_add(list, aux);
        aux = next;
    }
}

static struct sn_info *find_sn( struct sn_info *list, n2n_sock_t *sn )
{
    while (list)
    {
        if (!memcmp(&list->sn, sn, sizeof(n2n_sock_t)))
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

int update_sn_list( sn_list_t *list, n2n_sock_t *sn )
{
    struct sn_info *item = find_sn(list, sn);
    if (item)
    {
        return 0;
    }

    //TODO remove
    int item_size = sizeof(sn->family) + sizeof(sn->port);
    if (sn->family == AF_INET)
    {
        item_size += IPV4_SIZE;
    }
    else if (sn->family == AF_INET6)
    {
        item_size += IPV6_SIZE;
    }
    else
    {
        traceEvent(TRACE_ERROR,
                "unsupported family type for supernode address");
        return -1;
    }

    struct sn_info *new = calloc(1, sizeof(struct sn_info));
    if (!new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new SN info");
        return -1;
    }

    memcpy(&new->sn, sn, sizeof(n2n_sock_t));
    sn_list_add(&list->list_head, new);
    list->bin_size += item_size;

    return 0;
}

/*
 * Operations on comm_info lists.
 */
int read_comm_list_from_file( const char *filename, struct comm_info **list )
{
    struct comm_info *ci = NULL;

    FILE *fd = fopen(filename, "r");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file");
        return -1;
    }

    unsigned int comm_num = 0;
    if (fread(&comm_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't read communities number from file");
        goto out_err;
    }

    int i;
    for (i = 0; i < comm_num; i++)
    {
        ci = calloc(1, sizeof(struct comm_info));
        if (!ci)
        {
            traceEvent(TRACE_ERROR, "couldn't allocate a new community entry");
            goto out_err;
        }

        if (fread(&ci->sn_num, sizeof(size_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't read SN number from file");
            goto out_err;
        }
        if (fread(ci->community_name, sizeof(n2n_community_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't read community name from file");
            goto out_err;
        }

        comm_list_add(list, ci);
    }

    comm_reverse_list(list);

    fclose(fd);
    return 0;

    out_err: clear_comm_list(list);
    fclose(fd);
    return -1;
}

int write_comm_list_to_file( const char *filename, struct comm_info *list )
{
    FILE *fd = fopen(filename, "w");
    if (!fd)
    {
        traceEvent(TRACE_ERROR, "couldn't open community file");
        return -1;
    }

    unsigned int comm_num = comm_list_size(list);
    if (fwrite(&comm_num, sizeof(unsigned int), 1, fd) != 1)
    {
        traceEvent(TRACE_ERROR, "couldn't write community number to file");
        goto out_err;
    }

    while (list)
    {
        if (fwrite(&list->sn_num, sizeof(size_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't write SN number to file");
            goto out_err;
        }
        if (fwrite(list->community_name, sizeof(n2n_community_t), 1, fd) != 1)
        {
            traceEvent(TRACE_ERROR, "couldn't write community name to file");
            goto out_err;
        }
        list = list->next;
    }

    fclose(fd);
    return 0;

    out_err: fclose(fd);
    return -1;
}

/** Add new to the head of list. If list is NULL; create it.
 *
 *  The item new is added to the head of the list. New is modified during
 *  insertion. list takes ownership of new.
 */
void comm_list_add( struct comm_info **list, struct comm_info *new )
{
    new->next = *list;
    *list = new;
}

size_t clear_comm_list( struct comm_info **comm_list )
{
    size_t retval = 0;
    struct comm_info *scan = *comm_list;

    while (scan)
    {
        struct comm_info *crt = scan;
        scan = scan->next;
        free(crt);
        retval++;
    }
    *comm_list = NULL;

    return retval;
}

size_t comm_list_size( const struct comm_info *list )
{
    size_t retval = 0;
    while (list)
    {
        ++retval;
        list = list->next;
    }
    return retval;
}

void comm_reverse_list( struct comm_info **list )
{
    struct comm_info *aux = *list, *next = NULL;
    *list = NULL;

    while (aux)
    {
        next = aux->next;
        comm_list_add(list, aux);
        aux = next;
    }
}

static struct comm_info *find_comm( struct comm_info *list,
                                    n2n_community_t *comm_name,
                                    size_t comm_name_len )
{
    while (list)
    {
        if (!memcmp(list->community_name, comm_name, comm_name_len))
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

int update_comm_list( comm_list_t *comm_list, size_t sn_num,
                      snm_comm_name_t *community_name )
{
    struct comm_info *info = find_comm(comm_list->list_head,
            &community_name->name, community_name->size);
    if (info)
    {
        info->sn_num += sn_num;
        return 0;
    }

    struct comm_info *new = calloc(1, sizeof(struct comm_info));
    if (new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new community info");
        return -1;
    }

    memcpy(&new->community_name, community_name->name, community_name->size);
    new->sn_num = sn_num;

    comm_list_add(&comm_list->list_head, new);
    return 0;
}

int update_communities( comm_list_t *communities, n2n_community_t *comm_name )
{
    struct comm_info *info = find_comm(communities->list_head, comm_name,
            strlen((char *) comm_name));
    if (info)
    {
        return 0;
    }

    struct comm_info *new = calloc(1, sizeof(struct comm_info));
    if (new)
    {
        traceEvent(TRACE_ERROR, "not enough memory for new community info");
        return -1;
    }

    memcpy(&new->community_name, comm_name, sizeof(n2n_community_t));
    new->sn_num = 1;

    comm_list_add(&communities->list_head, new);
    return 0;
}

/*
 *
 */

static int snm_info_add_sn( n2n_SNM_INFO_t *info, struct sn_info *supernodes )
{
    info->sn_num = sn_list_size(supernodes);
    if (alloc_supernodes(&info->sn_ptr, info->sn_num))
    {
        traceEvent(TRACE_ERROR, "could not allocate supernodes array");
        return -1;
    }

    n2n_sock_t *sn = info->sn_ptr;

    while (supernodes)
    {
        memcpy(sn, &supernodes->sn, sizeof(n2n_sock_t));
        supernodes = supernodes->next;
        sn++;
    }
    return 0;
}
static int snm_info_add_comm( n2n_SNM_INFO_t *info,
                              struct comm_info *communities )
{
    info->comm_num = comm_list_size(communities);
    if (alloc_communities(&info->comm_ptr, info->comm_num))
    {
        traceEvent(TRACE_ERROR, "could not allocate communities array");
        return -1;
    }

    snm_comm_name_t *cni = info->comm_ptr;

    while (communities)
    {
        cni->size = strlen((char *) communities->community_name);
        memcpy(cni->name, communities->community_name, sizeof(n2n_community_t));
        communities = communities->next;
        cni++;
    }
    return 0;
}

/*
 * BLABLABLA
 */

/*int process_sn_msg( n2n_sn_t * sss,
 const struct sockaddr_in *sender_sock,
 const uint8_t *udp_buf,
 size_t udp_size
 time_t now)
 {
 return 0;
 }*/

/*
 * Response to request
 */
int build_snm_info( sn_list_t *supernodes, comm_list_t *communities,
                    snm_hdr_t *req_hdr, n2n_SNM_REQ_t *req,
                    n2n_SNM_INFO_t *info )
{
    int retval = 0;

    memset(info, 0, sizeof(n2n_SNM_INFO_t));

    if (GET_S(req_hdr->flags))
    {
        /* Set supernodes list */
        retval += snm_info_add_sn(info, supernodes->list_head);
    }
    if (GET_C(req_hdr->flags))
    {
        /* Set communities list */
        retval += snm_info_add_comm(info, communities->list_head);
    }
    else if (GET_N(req_hdr->flags))
    {
        /* Set supernodes???TODO */
    }

    return retval;
}

void clear_snm_info( n2n_SNM_INFO_t *info )
{
    info->sn_num = 0;
    free_supernodes(&info->sn_ptr);
    info->comm_num = 0;
    free_communities(&info->comm_ptr);
}

/*
 * Process response
 */
void process_snm_rsp( sn_list_t *supernodes, comm_list_t *communities,
                      n2n_SNM_INFO_t *snm_info )
{
    int i;

    int sn_num = sn_list_size(supernodes->list_head);

    /* Update list of supernodes */
    for (i = 0; i < snm_info->sn_num; i++)
    {
        update_sn_list(supernodes, &snm_info->sn_ptr[i]);

        if (sn_num != sn_list_size(supernodes->list_head))
        {
            /* elements added */
            write_sn_list_to_file(supernodes->filename, supernodes->list_head);
        }
    }

    /* Update list of communities from recvd from a supernode */
    for (i = 0; i < snm_info->comm_num; i++)
    {
        update_comm_list(communities, 1, &snm_info->comm_ptr[i]);
    }
}
