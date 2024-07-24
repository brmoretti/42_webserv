/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bmoretti <bmoretti@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/13 13:06:55 by bmoretti          #+#    #+#             */
/*   Updated: 2024/07/22 22:27:39 by bmoretti         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "common.hpp"
# include "Request.hpp"

namespace HttpStatus {
	enum Code {
		OK = 200,
		CREATED = 201,
		NO_CONTENT = 204,
		MOVED_PERMANENTLY = 301,
		BAD_REQUEST = 400,
		FORBIDDEN = 403,
		NOT_FOUND = 404,
		NOT_ALLOWED = 405,
		TIMEOUT = 408,
		CONFLICT = 409,
		PAYLOAD_TOO_LARGE = 413,
		SERVER_ERR = 500,
		NOT_IMPLEMENTED = 501,
		SERVICE_UNAVAILABLE = 503,
	};
};

typedef struct s_response
{
	std::string							statusLine;
	std::map<std::string, std::string>	headers;
	std::string							body;
} t_response;

class Response
{
public:
	Response(Request & request);
	~Response();

	std::string	getResponse() const;

private:
	Request &	_request;
	t_response	_response;

	void		_generateStatusLine();
	void		_generateHeaders();
	void		_generateBody();
	std::string	_generateResponse() const;

};

#endif
