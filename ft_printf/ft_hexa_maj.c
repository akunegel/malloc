/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_hexa_maj.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/23 20:08:17 by akunegel          #+#    #+#             */
/*   Updated: 2023/04/24 15:19:56 by akunegel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_hex_maj(unsigned int nb)
{
	char	*base;
	int		taille;

	taille = 0;
	base = "0123456789ABCDEF";
	if (nb > 15)
	{
		taille = ft_hex_maj(nb / 16);
		ft_hex_maj(nb % 16);
	}
	if (nb < 16)
	{
		ft_putchar(base[nb]);
	}
	return (taille + 1);
}
